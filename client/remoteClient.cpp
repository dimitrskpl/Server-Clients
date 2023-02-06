#include <stdio.h>
#include <sys/types.h> 
#include <sys/socket.h> 
#include <netinet/in.h> 
#include <unistd.h> 
#include <netdb.h> 
#include <stdlib.h> 
#include <string.h> 
#include <iostream>
#include <arpa/inet.h>
#include <fcntl.h>
#include <dirent.h>
#include <sys/stat.h>
#define PERMS 0666

using namespace std;

//check argv and save initilization client's info
//return 0 on success and -1 in case of wrong parameters
int get_server_info(int argc, char *argv[], struct in_addr *server_ip, int *server_port, char **dir);

//print initilization info of client
void print_server_info(struct in_addr server_ip, int server_port, char *dir);

void perror_exit(const char *message);

//creates folders in to_folder needed for path 
//and then creates file. if file with same name 
//exists delete it. Opens file and return fd on
//succes and -1 on error
int client_create(const char *path, const char *my_path);

//write block buffer in fd. Return bytes writen
//on success or -1 on error
int client_write(int fd, char *block_buffer);

//reads num from fd with sizeof int32_t using ntohl
//return bytes read on success or -1 on error
int read_num(int *num, int fd);

//read one char from fd return 0 
//on success or -1 on error
int read_type(int fd, char *type);

int main (int argc, char *argv[]){
    int sock, i;
    struct sockaddr_in server;
    struct hostent *hp;

    if(argc != 7){
        cout << "Please give server ip, port number and folder\n";
        exit(1);
    }

    struct in_addr server_ip;
    int server_port;
    char *dir;
    if(get_server_info(argc, argv, &server_ip, &server_port, &dir) == -1){
        cout << "Problem in parametres\n";
        exit(-1);
    }
    print_server_info(server_ip, server_port, dir);
    // Create socket 
    if((sock = socket(AF_INET, SOCK_STREAM, 0)) < 0)
        perror_exit("socket");

    if((hp = gethostbyaddr((const char*) &server_ip, sizeof(server_ip), AF_INET)) == NULL){
        herror("gethostbyaddr"); 
        exit(1);
    }
    server.sin_family = AF_INET; // Internet domain 
    memcpy(&server.sin_addr, hp->h_addr, hp->h_length);
    server.sin_port = htons(server_port); // Server port 
    // Initiate connection 
    if(connect(sock, (struct sockaddr*) &server, sizeof(server)) < 0)
        perror_exit("connect");
    cout << "Connecting to " << inet_ntoa(server_ip) << " on port " << server_port << endl;
    
    //ask for dir
    if(write(sock, dir, strlen(dir)) < 0)
        perror_exit("write");
    
    const char *to_folder = "results"; //the folder in which the results will be writen
    char type; //type of each msg
    char *block_buffer; //buffer to store files' content
    int block_sz; //the size of block server sending
    //read first msg F - block_sz
    if(read_type(sock, &type) < 0){
        delete dir;
        close(sock);
        perror_exit("client_create");
    }
    if(type == 'F'){
        if(read_num(&block_sz, sock) < 0){
            delete dir;
            close(sock);
            perror_exit("read_num");
        }
        block_buffer = new char[block_sz+1];
    }
    else{
        cout << "Expected type F\n";
        delete dir;;
        close(sock);
        exit(-1);
    }
    int fd; //file descriptor
    char *file_name;
    int nread;
    //read msg S/E/C/T - bytes_to_read - file_name/file_content
    while(1){
        if(read_type(sock, &type) < 0){
            delete dir;
            close(sock);
            perror_exit("read_type");
        }

        int bytes_to_read;
        if(read_num(&bytes_to_read, sock) < 0){
            delete dir;
            close(sock);
            perror_exit("read_num");
        }
        //S - bytes_to_read - file_name 
        if(type == 'S'){
            file_name = new char[bytes_to_read+1];
            if((nread = read(sock, file_name, bytes_to_read)) < 0){
                delete dir;
                close(sock);
                perror_exit("read");          
            } 
            file_name[nread] = '\0';
            //create neccessary folder and file 
            if((fd = client_create(file_name, to_folder)) < 0){
                delete dir;
                close(sock);
                perror_exit("client_create");
            }           
        }
        else if(type == 'T' || type == 'C' || type == 'E'){
            //read content
            if(bytes_to_read != 0){
                if((nread = read(sock, block_buffer, bytes_to_read)) < 0){
                    delete dir;
                    close(fd);
                    close(sock);
                    perror_exit("read");
                }
                block_buffer[nread] = '\0';
                if(client_write(fd, block_buffer) < 0){
                    delete dir;
                    close(fd);
                    close(sock);
                    perror_exit("client_write");
                }
            }

            //termination (last content)
            if(type == 'T'){
                cout << "Received: " << file_name << endl;
                delete file_name;
                close(fd);
                break;
            }
            else if(type == 'E'){ //end of file (last content of file)
                cout << "Received: " << file_name << endl;
                close(fd);
                delete file_name;
            }
        }
        else{
            cout << "Cannot recognize message type\n";
            delete block_buffer;
            delete dir;
            close(fd);
            close(sock);
            exit(-1);
        }
    }

    delete block_buffer;
    delete dir;
    close(sock); 
    return 0;
}


int client_create(const char *path, const char *to_folder){ //
    char *token;
    char *str = strdup(path); //copy path
    token = strtok_r(str, "/", &str); //get first token until '/'
    string new_path(to_folder);
    string slash = "/";
    string file;
    while(token != NULL){
        string token_str(token);
        // new_path/first_token
        new_path = new_path + slash + token_str; 
        if((token = strtok_r(NULL, "/", &str)) == NULL)
            break;
        struct stat st; 
        const char *folder_path_str = new_path.c_str();
        if(stat(folder_path_str, &st) == -1) //if folder doesnt exist
            mkdir(folder_path_str, 0700); //create folder
    }
    const char *path_file = new_path.c_str();
    if(access(path_file, F_OK) == 0) //if file exists
        remove(path_file); //delete file

    int fd;
    //create and open file
    if((fd = open(path_file, O_CREAT | O_WRONLY , PERMS)) < 0){
        perror("open");
        return -1;
    }       
    return fd;
}

int client_write(int fd, char *block_buffer){
    int nwrite;
    size_t block_buffer_sz = strlen(block_buffer);
    if((nwrite = write(fd, block_buffer, block_buffer_sz)) < 0)
        perror("write");
    return nwrite;
}

void perror_exit(const char *message){
    perror(message);
    exit(EXIT_FAILURE);
}


int get_server_info(int argc, char *argv[], struct in_addr *server_ip, int *server_port, char **dir){
    for(int i = 1; i < argc; i+=2){
        if(!strcmp(argv[i], "-i")){
            inet_aton(argv[i+1], server_ip);
        }
        else if(!strcmp(argv[i], "-p")){ 
            *server_port = atoi(argv[i+1]);
        }
        else if(!strcmp(argv[i], "-d")){
            int dir_len = strlen(argv[i+1]);
            *dir = new char[dir_len+1];
            strcpy(*dir, argv[i+1]);
        }
        else{
            return -1;
        }
    }
    return 0;
}

void print_server_info(struct in_addr server_ip, int server_port, char *dir){
    cout << "Client's parameters are:\n";
    cout << "serverIP: " << inet_ntoa(server_ip) << endl;
    cout << "port: " << server_port << endl;
    cout << "directory: " << dir << endl;
}


int read_num(int *num, int fd){
    int32_t int32_num;
    char *data = (char*)&int32_num;
    int num_sz = sizeof(int32_num);
    int nread;
    nread = read(fd, data, num_sz);
    *num = ntohl(int32_num);
    return nread;
}


int read_type(int fd, char *type){
    if(read(fd, type, 1) < 0)
        return -1;
    return 0;
}



