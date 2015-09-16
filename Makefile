default: server

server: server.o
	gcc -g -Wall -o server server.o

server.o: server.c
	gcc -g -Wall -c server.c

clean:
	rm server.o
	rm server
