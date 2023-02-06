#pragma once

#include <string>
#include <pthread.h>

using namespace std;

typedef struct{
  string f_name; //file name
  unsigned int f_size; //size of file in bytes
  unsigned int socket; //socket descriptor
}file_info;

typedef struct{
    unsigned int total_files; //total files contained in dir asked from client
    unsigned int files_sent; //no of files sent to client
    int chars_to_remove; //indicates the no of chars to remove from path to file name
    pthread_mutex_t *socket_mtx; //mtx for sending to socket control
}socket_info;