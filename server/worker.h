#include <pthread.h>
#include <map>
#include <queue>
#include "common_types.h"


typedef struct{
    unsigned int block_sz;
    map<int, socket_info*> *socket_folder_map; //map socket descriptor to socket_info, contain one socket info for each client
    queue<file_info> *file_queue; //execution queue common to all threads containing files info
    pthread_mutex_t *map_mtx; //mtx for map function calls
    pthread_mutex_t *queue_mtx; //mtx for get/place control in file_queue
    pthread_cond_t *cond_nonempty; //cond var checking if file_queue is empty
    pthread_cond_t *cond_nonfull; //cond var checking if file_queue is full
}worker_info;

//initialize worker_info
void worker_info_init(worker_info *w_info, unsigned int block_sz, map<int, socket_info*> *socket_folder_map, queue<file_info> *file_queue,
    pthread_mutex_t *map_mtx, pthread_mutex_t *queue_mtx, pthread_cond_t *cond_nonempty, pthread_cond_t *cond_nonfull);

//worker function
void *worker(void *argp);

//type: T->termination (last content), E->end of file (last content of file), C->only content (continue)
//return 'T' if total_bytes == bytes_sent and total_files==files_sent else
//return 'E' if only total_bytes == bytes_sent else return 'C'
char msg_type(unsigned int total_bytes, unsigned int bytes_sent, unsigned int total_files, unsigned int files_sent);

//write to each element from from_deque the socket
//and insert them to file_queue keeping the order.
//Wait cond_nonempty if file_queue full and signal 
//cond_nonfullif file_queue is not empty

//wait cond_nonempty until file_queue is not empty.
//get first f_info from file_queue and remove it,
//signal cond_nonfull if file_queue is not full
void get_file_info(file_info *f_info, pthread_mutex_t *queue_mtx, queue<file_info> *file_queue, pthread_cond_t *cond_nonempty, pthread_cond_t *cond_nonfull);

//write int num to fd using htnol
int write_num(int num, int fd);

//write to fd type, bytes_to_read and string msg 
//if msg is not NULL. On success return 0, else -1
int write_msg(char type, int bytes_to_read, const char *msg, int fd);

//send integer block_sz to fd.
//On success return 0, else -1
int write_first_msg(int block_sz, int fd);
