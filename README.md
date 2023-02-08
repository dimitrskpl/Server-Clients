# Server-Clients
An ftp‑like server supporting multiple clients communicating through sockets.

The purpose is to copy all the contents of one directory recursively from a server, to the local file system of the client.
The server is able to handle requests from different clients at any time and process each request in parallel, breaking it into independent operations of copying files. Accordingly, the client processes the data being sent from the server and finally, creates a local copy of the requested directory. This directory contains exactly the same structure and files as that of the server. 

Many clients can connect to a server at the same time. Communication between them is done by sockets.

**Server:** The server creates a new communication thread for each new connection it accepts, which is responsible for reading from the client the name of the directory to copy. The directory to be copied will be read from the server's local file system recursively. Each file in the hierarchy will be placed in one queue that will be common to all threads. The queue has specific size, which is given as an argument. In case the queue is full, the communication thread serving the client waits, until free space is created.

The server has a thread pool, which consists of worker threads. The size of the thread pool is given as argument. The thread pool assigns to a worker thread, a file to read. In case the queue is empty, the worker threads waits until there is some entry in the queue. Each worker thread is responsible for reading the contents of the file and for sending them to the corresponding client, through the corresponding socket. The file is read and is sent to the client divided in blocks. The block size is given as an argument in bytes. Finally, during server's execution only one worker thread at a time writes data at the socket of each client.

**Client:** More than one client can be active in the system at the same time. Each client connects to the server on the port given as an argument, and specifies the name of the directory (as an argument) it wants to copy. Then, for each file contained in the directory the server returns : a) the name of the file b) any metadata of the file and c) the contents of the file. The file name is in path format and the client creates the appropriate directories and subdirectories. In case one file already exists on the client's local file system, this file is deleted and created from scratch (not overwritten).

It is claimed that: 
* the client knows the server's file system hierarchy and can select any directory to copy.
* the server's file system does not contain empty directories.

## Usage
For the server go to server/ and for the client go to client/.
The compilation command for each of them is:
```
$ make all
```
The execution command for the server is:
```
$ ./dataServer -p <port> -s <thread_pool_size> -q <queue_size> -b <block_size>
```
* port: The port on which the server will listen for external connections.
* thread_pool_size: The size of the thread pool.
* queue_size: The size of the queue.
* block_size: The size of the file blocks in bytes that the worker threads will send.

The execution command for the client is:
```
$ ./remoteClient -i <server_ip> -p <server_port> -d <directory>
```

* server_ip: The IP address of the server
* server_port: The port on which the server listens for external connections
* directory: The directory to copy (the relative path).

An example:
```
$ ./dataServer -p 2080 -s 10 -q 10 -b 128
Servers' parameters are:
port: 2080
thread pool size: 10
queue_size: 10
Block size: 128
Server was succesfully initialized...
Listening for connections to port 2080
Accepted connection
[Thread: 140606089135872]: About to scan directory: output_dir/Server-Clients
[Thread 140606089135872]: Adding file output_dir/Server-Clients/client/Makefile to the queue...
[Thread 140606089135872]: Adding file output_dir/Server-Clients/client/remoteClient.cpp to the queue...
[Thread 140606089135872]: Adding file output_dir/Server-Clients/client/results/.placeholder to the queue...
[Thread 140606089135872]: Adding file output_dir/Server-Clients/server/commun.cpp to the queue...
[Thread 140606089135872]: Adding file output_dir/Server-Clients/server/commun.h to the queue...
[Thread 140606382016256]: Received task <output_dir/Server-Clients/client/Makefile, 4>
[Thread 140606089135872]: Adding file output_dir/Server-Clients/server/dataServer.cpp to the queue...
[Thread 140606089135872]: Adding file output_dir/Server-Clients/server/Makefile to the queue...
[Thread 140606089135872]: Adding file output_dir/Server-Clients/server/output_dir/.placeholder to the queue...
[Thread 140606339745536]: Received task <output_dir/Server-Clients/client/remoteClient.cpp, 4>
[Thread 140606373562112]: Received task <output_dir/Server-Clients/client/results/.placeholder, 4>
[Thread 140606356653824]: Received task <output_dir/Server-Clients/server/commun.cpp, 4>
[Thread 140606089135872]: Adding file output_dir/Server-Clients/server/worker.h to the queue...
[Thread 140606365107968]: Received task <output_dir/Server-Clients/server/commun.h, 4>
[Thread 140606348199680]: Received task <output_dir/Server-Clients/server/dataServer.cpp, 4>
[Thread 140606331291392]: Received task <output_dir/Server-Clients/server/Makefile, 4>
[Thread 140606322837248]: Received task <output_dir/Server-Clients/server/output_dir/.placeholder, 4>
[Thread 140606305928960]: Received task <output_dir/Server-Clients/server/worker.h, 4>
[Thread 140606382016256]: About to read file output_dir/Server-Clients/client/Makefile
[Thread 140606339745536]: About to read file output_dir/Server-Clients/client/remoteClient.cpp
[Thread 140606373562112]: About to read file output_dir/Server-Clients/client/results/.placeholder
[Thread 140606356653824]: About to read file output_dir/Server-Clients/server/commun.cpp
[Thread 140606365107968]: About to read file output_dir/Server-Clients/server/commun.h
[Thread 140606348199680]: About to read file output_dir/Server-Clients/server/dataServer.cpp
[Thread 140606331291392]: About to read file output_dir/Server-Clients/server/Makefile
[Thread 140606322837248]: About to read file output_dir/Server-Clients/server/output_dir/.placeholder
[Thread 140606305928960]: About to read file output_dir/Server-Clients/server/worker.h
```

```
$ ./remoteClient -i 127.0.0.1 -p 2080 -d output_dir/Server-Clients
Client's parameters are:
serverIP: 127.0.0.1
port: 2080
directory: output_dir/Server-Clients
Connecting to 127.0.0.1 on port 2080
Received: Server-Clients/client/Makefile
Received: Server-Clients/client/remoteClient.cpp
Received: Server-Clients/client/results/.placeholder
Received: Server-Clients/server/commun.cpp
Received: Server-Clients/server/commun.h
Received: Server-Clients/server/dataServer.cpp
Received: Server-Clients/server/Makefile
Received: Server-Clients/server/output_dir/.placeholder
Received: Server-Clients/server/worker.h
```
