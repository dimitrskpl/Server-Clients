# Client-Server-Content-Delivery
An ftp‑like server supporting multiple clients communicating through sockets

The purpose is to copy all the contents of one directory recursively from a server, to the local file system of the client.
The server is able to handle requests from different clients at any time and process each request in parallel, breaking it into independent operations of copying files. Accordingly, the client processes the data being sent from the server and finally, creates a local copy of the requested directory. This directory contains exactly the same structure and files as that of the server. 

Many clients can connect to a server at the same time. Communication between them is done by sockets.

**Server:** The server creates a new communication thread for each new connection it accepts, which is responsible for reading from the client the name of the directory to copy. The directory to be copied will be read from the server's local file system recursively. Each file in the hierarchy will be placed in one queue that will be common to all threads. The queue has specific size, which is given as an argument. In case the queue is full, the communication thread serving the client waits, until free space is created.

The server has a thread pool, which consists of worker threads. The size of the thread pool is given as argument. The thread pool assigns to a worker thread, a file to read. In case the queue is empty, the worker threads waits until there is some entry in the queue. Each worker thread is responsible to read the contents of the file and send them to the corresponding client, through the corresponding socket. The file is read and is sent to the client divided in blocks. The block size is given as an argument in bytes. Finally, during server's execution only one worker thread at a time writes data at the socket of each client, until it finishes the writting.

**Client:** More than one client can be active in the system at the same time. Each client connects to the server on the port given as an argument, and specifies the name of the directory (as an argument) it wants to copy. Then, for each file contained in the directory the server returns : a) the name of the file b) any metadata of the file and c) the contents of the file. The file name is in path format and the client creates the appropriate directories and subdirectories. In case one file already exists on the client's local file system, this file is deleted and created from scratch (not overwritten).

It is claimed that: 
* the client knows the server's file system hierarchy and can select any directory to copy.
* the server's file system does not contain empty directories

## **Command Line Arguments**
