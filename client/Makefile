OBJS = remoteClient.o 
SOURCE = remoteClient.cpp 
HEADER = 
EXEC = remoteClient
CC = g++
FLAGS = -c

all: $(EXEC)

$(EXEC) : $(OBJS)
	${CC} -o $@ ${OBJS}

remoteClient.o: remoteClient.cpp
	$(CC) $(FLAGS) remoteClient.cpp

#clean objects and executable
clean:
	rm -f $(OBJS) $(EXEC)

run: $(EXEC)
	./$(EXEC)