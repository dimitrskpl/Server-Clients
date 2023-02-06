#include "commun.h"   
#include <fcntl.h>
#include <iostream> 
#include <unistd.h>
#include <string>
#include <string.h>
#include <dirent.h>
#include <sys/stat.h>
 
using namespace std;

void commun_info_init(commun_info *c_info, unsigned int max_queue_sz, unsigned int socket, pthread_mutex_t *queue_mtx, 
    pthread_mutex_t *map_mtx, pthread_cond_t *cond_nonempty, pthread_cond_t *cond_nonfull, 
    queue<file_info> *file_queue, map<int, socket_info*> *socket_folder_map){

        c_info->max_queue_sz = max_queue_sz;
        c_info->socket = socket;
        c_info->cond_nonempty = cond_nonempty;
        c_info->cond_nonfull = cond_nonfull;
        c_info->file_queue = file_queue;
        c_info->map_mtx = map_mtx;
        c_info->queue_mtx = queue_mtx;
        c_info->socket_folder_map = socket_folder_map;
}

void *communicate(void *argp){
    //save argp info
    commun_info *c_info = (commun_info*) argp;
    int newsock = c_info->socket;
    unsigned int max_queue_sz = c_info->max_queue_sz;
    unsigned int socket = c_info->socket;
    pthread_mutex_t *queue_mtx = c_info->queue_mtx;
    pthread_mutex_t *map_mtx = c_info->map_mtx;
    pthread_cond_t *cond_nonempty = c_info->cond_nonempty;
    pthread_cond_t *cond_nonfull = c_info->cond_nonfull;
    queue<file_info> *file_queue = c_info->file_queue;
    map<int, socket_info*> *socket_folder_map = c_info->socket_folder_map;

    char dir[256]; //directory name cant be greater than 256 chars
    int nread;
    //read name of directory (with relative path)
    if((nread = read(newsock, dir, 256)) <= 0){
        perror("read");
        exit(-1);
    }
    dir[nread] = '\0';
    cout << "[Thread: " << pthread_self() <<"]: About to scan directory: " << dir << endl;
    deque<file_info> socket_file_deque; //deque for files info
    if(visitdir(dir, &socket_file_deque) == -1) //save files name and files size to deque
        perror("visitdir");
    int last_slash_chars = chars_after_last_slash(dir);
    int chars_to_remove = 0;
    if(last_slash_chars != -1) //there is slash in given dir
        chars_to_remove = strlen(dir) - last_slash_chars; //no of chars to remove when inserting file name
    socket_info *cur_s_info = new socket_info; //will be deleted from worker when sent last message
    socket_info_init(cur_s_info, socket_file_deque.size(), chars_to_remove);
    
    pthread_mutex_lock(map_mtx);
    socket_folder_map->insert(pair<int, socket_info*> (newsock, cur_s_info));
    pthread_mutex_unlock(map_mtx);

    place_files_info(socket_file_deque, max_queue_sz, newsock, cond_nonempty, cond_nonfull, file_queue, queue_mtx);
    delete c_info;
    pthread_exit(0);
}

int visitdir(const char *name, deque<file_info> *f_queue){
    DIR *dir;
    if((dir = opendir(name)) == NULL) 
        return -1;

    struct dirent *dir_entry;
    while(dir_entry = readdir(dir)){
        string slash = "/";
        string cur_name(name);
        string next_name(dir_entry->d_name);
        string path_file = cur_name + slash + next_name;

        if(dir_entry->d_type == DT_REG){ //file
            struct stat buf;
            if(stat(path_file.c_str(), &buf)==-1)//save file attributes to buf
                return -1;
            file_info cur_file;
            cur_file.f_name = path_file;
            cur_file.f_size = buf.st_size; //get file size
            f_queue->push_back(cur_file);
        } 
        else if(dir_entry->d_type == DT_DIR){ //directory
            //ignore these directories
            if (strcmp(dir_entry->d_name, ".") == 0 || strcmp(dir_entry->d_name, "..") == 0)
                continue;

            if(visitdir(path_file.c_str(), f_queue) == -1) //visit subtree
                return -1;
        }
    }
    closedir(dir);
    return 0;
}


void place_files_info(deque<file_info> from_deque, int max_queue_sz, unsigned int socket, pthread_cond_t *cond_nonempty, pthread_cond_t *cond_nonfull, queue<file_info> *file_queue, pthread_mutex_t *queue_mtx){
    for (int i = 0; i < from_deque.size(); i++) {
        file_info cur_f_info;
        cur_f_info.f_name = from_deque[i].f_name;
        cur_f_info.f_size = from_deque[i].f_size;
        cout << "[Thread " << pthread_self()<< "]: Adding file " << cur_f_info.f_name << " to the queue...\n";
        pthread_mutex_lock(queue_mtx);
        while(file_queue->size() >= max_queue_sz){
            pthread_cond_wait(cond_nonfull, queue_mtx);
        }
        cur_f_info.socket = socket;
        file_queue->push(cur_f_info);
        pthread_mutex_unlock(queue_mtx);
        pthread_cond_signal(cond_nonempty);
    }
}


int chars_after_last_slash(char* str){
    unsigned int length = strlen(str);
    int cnt = 0;
    int i = length-1;
    while(str[i] != '/' && i != -1){
        cnt++;
        i--;
    }
    if(i == -1)
        return -1; //not found
    return cnt;
}

void socket_info_init(socket_info* s_info, int total_files, int chars_to_remove){
    s_info->files_sent = 0;
    s_info->total_files = total_files;
    s_info->chars_to_remove = chars_to_remove;
    s_info->socket_mtx = new pthread_mutex_t;
    pthread_mutex_init(s_info->socket_mtx , 0);
}
