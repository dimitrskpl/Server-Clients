#include <pthread.h>
#include <string.h>  
#include <iostream>
#include <fcntl.h>
#include <unistd.h>
#include <arpa/inet.h>
#include "worker.h" 
 
using namespace std;

void worker_info_init(worker_info *w_info, unsigned int block_sz, map<int, socket_info*> *socket_folder_map, queue<file_info> *file_queue,
    pthread_mutex_t *map_mtx, pthread_mutex_t *queue_mtx, pthread_cond_t *cond_nonempty, pthread_cond_t *cond_nonfull){

    w_info->block_sz = block_sz;
    w_info->cond_nonempty = cond_nonempty;
    w_info->cond_nonfull = cond_nonfull;
    w_info->map_mtx = map_mtx;
    w_info->queue_mtx = queue_mtx;
    w_info->socket_folder_map = socket_folder_map;
    w_info->file_queue = file_queue;
}

void *worker(void *argp){
    worker_info *w_info = (worker_info*)argp;
    unsigned int block_sz = w_info->block_sz;
    map<int, socket_info*> *socket_folder_map = w_info->socket_folder_map;
    pthread_mutex_t *map_mtx = w_info->map_mtx;
    pthread_mutex_t *queue_mtx = w_info->queue_mtx;
    pthread_cond_t *cond_nonempty = w_info->cond_nonempty;
    pthread_cond_t *cond_nonfull = w_info->cond_nonfull;
    queue<file_info> *file_queue = w_info->file_queue;
    
    char *block_buffer = new char[block_sz+1]; //plus 2: 1 for '\0' and 1 for message type
    while(1){
        file_info file_to_send;
        get_file_info(&file_to_send, queue_mtx, file_queue, cond_nonempty, cond_nonfull);
        string f_name = file_to_send.f_name;
        unsigned int f_size = file_to_send.f_size;
        unsigned int socket = file_to_send.socket;
        
        map<int,socket_info*>::iterator iter;
        iter = socket_folder_map->find(socket);
        if(iter == socket_folder_map->end()){
            perror("map_find_socket");
            pthread_exit(0);
        }
        socket_info *s_info = iter->second;

        pthread_mutex_lock(s_info->socket_mtx);
        s_info->files_sent++; //one more file to sent
        ////////write first message F-block_sz/////////
        if(s_info->files_sent == 1){
            if(write_first_msg(block_sz, socket) < 0){
                perror("write_to_socket");
                pthread_exit(0);
            }
        }
        ////////write start message S - bytes_to_read - file_name/////////
        int fd;
        if((fd = open(f_name.c_str(), O_RDONLY)) == -1){
            perror("worker_server");
            cout << "fail to open " << f_name.c_str() << endl;
            pthread_exit(0);
        }
        int bytes_to_read = strlen(f_name.c_str());
        if(write_msg('S', bytes_to_read, f_name.substr(s_info->chars_to_remove).c_str(), socket) < 0){
            perror("write_to_socket");
            pthread_exit(0);
        }

        int bytes_sent = 0;
        int nwrite;
        char type;
        /////////read and write contents' msg C - bytes_to_read - block_buffer /////////
        cout << "[Thread " << pthread_self() << "]: About to read file " << f_name << endl;
        int nread;
        while((nread = read(fd, block_buffer, block_sz)) > 0){
            block_buffer[nread] = '\0';
            bytes_sent += nread;
            type = msg_type(f_size, bytes_sent, s_info->total_files, s_info->files_sent);
            if(write_msg(type, nread, block_buffer, socket) < 0){
                perror("write_to_socket");
                close(fd);
                pthread_exit(0);
            }
        }
        close(fd);
        if(nread<0){
            perror("worker_read");
            pthread_exit(0);
        }
        //no content
        if(f_size == 0){
            ///////////write end or terminate msg E/T - 0////////////
            type = msg_type(0, 0, s_info->total_files, s_info->files_sent);
            if(write_msg(type, 0, NULL, socket) < 0){
                perror("write_to_socket");
                pthread_exit(0);
            }
        }
        pthread_mutex_unlock(s_info->socket_mtx);
        //last msg sent delete allocated space s_info from map
        //destroy socket_mtx
        if(type == 'T'){
            close(socket);
            pthread_mutex_destroy(s_info->socket_mtx);
            delete s_info->socket_mtx;
            delete s_info;
            pthread_mutex_lock(map_mtx);
            socket_folder_map->erase(iter);
            pthread_mutex_unlock(map_mtx);
        }
    }
    delete block_buffer;
}

void get_file_info(file_info *f_info, pthread_mutex_t *queue_mtx, queue<file_info> *file_queue, pthread_cond_t *cond_nonempty, pthread_cond_t *cond_nonfull){
    pthread_mutex_lock(queue_mtx);
    while(file_queue->empty())
        pthread_cond_wait(cond_nonempty, queue_mtx); 

    f_info->f_name = file_queue->front().f_name;
    f_info->f_size = file_queue->front().f_size;
    f_info->socket = file_queue->front().socket;
    
    cout << "[Thread " << pthread_self() << "]: Received task <" << f_info->f_name << ", " << f_info->socket << ">\n";
    file_queue->pop();
    pthread_mutex_unlock(queue_mtx);
    pthread_cond_signal(cond_nonfull);
}

char msg_type(unsigned int total_bytes, unsigned int bytes_sent, unsigned int total_files, unsigned int files_sent){
    if(total_bytes == bytes_sent){
        if(total_files==files_sent)
            return 'T';
        else
            return 'E';
    }
    else{ 
        return 'C';
    }
}

int write_num(int num, int fd){
    int32_t int32_num = htonl(num);
    char *num_str = (char*)&int32_num;
    int num_sz = sizeof(int32_num);
    int nwrite;
    nwrite = write(fd, num_str, num_sz);
    return nwrite;
}


int write_msg(char type, int bytes_to_read, const char *msg, int fd){
    if(write(fd, &type, 1) < 0)
        return -1;
    if(write_num(bytes_to_read, fd) < 0)
        return -1;
    if(msg != NULL){
        if(write(fd, msg, bytes_to_read) < 0)
            return -1;
    }
    return 0;
}

int write_first_msg(int block_sz, int fd){
    if(write(fd, "F", 1) < 0)
        return -1;
    if(write_num(block_sz, fd) < 0)
        return -1;
    return 0;
}