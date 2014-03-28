CFLAGS="-ggdb"
#CFLAGS="-pg"

server: lock_free_hashtable.o socket_server.o readqueue.o
	g++ $(CFLAGS) -std=gnu++0x -o $@ $^ -levent -lpthread

socket_server.o: socket_server.cc readqueue.c readqueue.h
	g++ $(CFLAGS) -std=gnu++0x -c socket_server.cc -levent -lpthread

readqueue.o: readqueue.c
	gcc $(CFLAGS) -c readqueue.c -levent -lpthread

lock_free_hashtable.o: lock_free_hashtable.cc
	g++ $(CFLAGS) -std=gnu++0x -c $^

.PHONY: clean
clean:
	-rm -f *.o server core
	

