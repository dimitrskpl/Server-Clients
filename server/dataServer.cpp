#include <sys/socket.h> 
#include <netinet/in.h> 
#include <string.h>
#include <iostream>
#include <netdb.h>
#include "worker.h"
#include "commun.h"

using namespace std;
 
//check argv and save initilization servers' info
//return 0 on success and -1 in case of wrong parameters
int get_server_info(int argc, char *argv[], int *port, int *thread_pool_sz, int *queue_sz, int *block_sz);

//print initilization info of server
void print_server_info(int port, int thread_pool_sz, int queue_sz, int block_sz);

void perror_exit(const char *message);

int main(int argc ,char *argv []){
    if(argc != 9){
        cout << "Please give port, thread_pool_size, queue_size and block_size\n";
        exit(1);
    }
    int port, thread_pool_sz, queue_sz, block_sz;
    if(get_server_info(argc, argv, &port, &thread_pool_sz, &queue_sz, &block_sz) == -1){
        cout << "Problem in parametres\n";
        exit(-1);
    }
    print_server_info(port, thread_pool_sz, queue_sz, block_sz);

    int lsock, csock;
    // Create socket 
    if((lsock = socket(AF_INET, SOCK_STREAM, 0)) < 0)
        perror_exit("socket");
    struct sockaddr_in server;
    server.sin_family = AF_INET; // Internet domain 
    server.sin_addr.s_addr = htonl(INADDR_ANY);
    server.sin_port = htons(port); // The given port 

    // Bind socket to address 
    if(bind(lsock, (struct sockaddr*)&server, sizeof(server)) < 0)
        perror_exit("bind");

    // Listen for connections 
    if(listen(lsock, 10) < 0) 
        perror_exit("listen");

    cout << "Listening for connections to port " << port << endl;
    //mutex and conds for file_queue place and get control
    pthread_mutex_t queue_mtx, map_mtx; //mutex for control in file_queue place/get contol and for map function calls
    pthread_cond_t cond_nonempty, cond_nonfull; //cond vars for checking file_queue full or empty

    queue<file_info> file_queue; //execution queue common to all threads
    map<int, socket_info*> socket_folder_map; //commun thread insert w_info for the new client and worker erase in last sent to client
    pthread_t communic_thread; //communication thread

    pthread_mutex_init(&queue_mtx , 0);
    pthread_mutex_init(&map_mtx , 0);
    pthread_cond_init(&cond_nonempty, 0);
    pthread_cond_init(&cond_nonfull, 0);
    worker_info w_info;
    worker_info_init(&w_info, block_sz, &socket_folder_map, &file_queue, &map_mtx, &queue_mtx, &cond_nonempty, &cond_nonfull);
    pthread_t *worker_threads = new pthread_t[thread_pool_sz]; //create a vector of thread_pool_sz threads
    //start worker threads
    for(int i = 0; i < thread_pool_sz; i++){
        pthread_create(&worker_threads[i], NULL, worker, &w_info);
    }


    while(1){
        // accept connection 
        if((csock = accept(lsock, NULL, NULL)) < 0){ 
            perror_exit("accept");
        }
        
        printf ("Accepted connection\n"); 

        commun_info *c_info = new commun_info; //will be deleted from commun_thread before exit
        commun_info_init(c_info, queue_sz, csock, &queue_mtx, &map_mtx, &cond_nonempty, &cond_nonfull, &file_queue, &socket_folder_map);
        //create new communication thread giving c_info
        pthread_create(&communic_thread, NULL, communicate, c_info); 
    }
}

int get_server_info(int argc, char *argv[], int *port, int *thread_pool_sz, int *queue_sz, int *block_sz){
    for(int i = 1; i < argc; i+=2){
        if(!strcmp(argv[i], "-p")){
            *port = atoi(argv[i+1]);
        }
        else if(!strcmp(argv[i], "-s")){
            *thread_pool_sz = atoi(argv[i+1]);
        }
        else if(!strcmp(argv[i], "-q")){
            *queue_sz = atoi(argv[i+1]);
        }
        else if(!strcmp(argv[i], "-b")){
            *block_sz = atoi(argv[i+1]);
        }
        else{
            return -1;
        }
    }
    return 0;
}

void print_server_info(int port, int thread_pool_sz, int queue_sz, int block_sz){
    cout << "Servers' parameters are:\n";
    cout << "port: " << port << endl;
    cout << "thread pool size: " << thread_pool_sz << endl;
    cout << "queue_size: " << queue_sz << endl;
    cout << "Block size: " << block_sz << endl;
    cout << "Server was succesfully initialized...\n";
}


void perror_exit(const char *message){
    perror(message);
    exit(EXIT_FAILURE);
}
