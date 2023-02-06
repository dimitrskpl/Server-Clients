#include <pthread.h>
#include <queue>
#include <map>
#include "common_types.h"

using namespace std;

typedef struct{
    unsigned int max_queue_sz; //max size of files_queue
    unsigned int socket; //socket descriptor
    pthread_mutex_t *queue_mtx; //mtx for get/place control in file_queue
    pthread_mutex_t *map_mtx; //mtx for map function calls
    pthread_cond_t *cond_nonempty; //cond var checking if file_queue is empty
    pthread_cond_t *cond_nonfull; //cond var checking if file_queue is full
    queue<file_info> *file_queue; //execution queue common to all threads containing files info
    map<int, socket_info*> *socket_folder_map; //map socket descriptor to socket_info, contain one socket info for each client
}commun_info;

//initialize commun_info
void commun_info_init(commun_info *c_info, unsigned int max_queue_sz, unsigned int socket, pthread_mutex_t *queue_mtx, 
    pthread_mutex_t *map_mtx, pthread_cond_t *cond_nonempty, pthread_cond_t *cond_nonfull, 
    queue<file_info> *file_queue, map<int, socket_info*> *socket_folder_map);

//initialize socket info
void socket_info_init(socket_info* s_info, int total_files, int chars_to_remove);

//visit subfolders and files of name retrospectively
//and insert file_name (with relative path) and file size
//to f_queue. On success return 0, else -1
int visitdir(const char *name, deque<file_info> *file_queue);

//communication thread function. Read from socket 
//dir name, visit folder tree and insert file info
//to file_queue
void *communicate(void *argp);

//return the no of chars after last slash
//or -1 if no slash exists
int chars_after_last_slash(char* str);

//write to each element from from_deque the socket
//and insert them to file_queue keeping the order.
//Wait cond_nonempty if file_queue full and signal 
//cond_nonfullif file_queue is not empty
void place_files_info(deque<file_info> from_queue, int max_queue_sz, unsigned int socket, pthread_cond_t *cond_nonempty, pthread_cond_t *cond_nonfull, queue<file_info> *file_queue, pthread_mutex_t *queue_mtx);
