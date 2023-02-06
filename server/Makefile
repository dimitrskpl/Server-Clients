OBJS = dataServer.o worker.o commun.o
SOURCE = dataServer.cpp worker.cpp commun.cpp
HEADER = worker.h common_types.h commun.h
EXEC = dataServer
CC = g++
FLAGS = -c  
LDFLAGS = -lpthread

all: $(EXEC)

$(EXEC) : $(OBJS)
	$(CC) $(OBJS) -o $(EXEC) $(LDFLAGS)

dataServer.o: dataServer.cpp worker.cpp commun.cpp
	$(CC) $(FLAGS) dataServer.cpp
	$(CC) $(FLAGS) worker.cpp
	$(CC) $(FLAGS) commun.cpp

worker.o: worker.cpp
	$(CC) $(FLAGS) worker.cpp

commun.o: commun.cpp
	$(CC) $(FLAGS) commun.cpp

#clean objects and executable
clean:
	rm -f $(OBJS) $(EXEC)

run: $(EXEC)
	./$(EXEC)