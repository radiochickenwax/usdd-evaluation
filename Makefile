default: server


serialExample: serialExample.c
	gcc -g -Wall  -o serialExample serialExample.c

server: server.o
	gcc -g -Wall -o server server.o

server.o: server.c
	gcc -g -Wall -c server.c

clean:
	rm server.o
	rm server
