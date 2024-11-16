all: server client other

server: server.o utility.o cmd_client.o gioco.o
	gcc -Wall server.o utility.o cmd_client.o gioco.o -o server

client: client.o utility.o cmd_client.o gioco.o
	gcc -Wall client.o utility.o cmd_client.o gioco.o -o client

other: other.o utility.o cmd_client.o gioco.o
	gcc -Wall other.o utility.o cmd_client.o gioco.o -o other


utility.o: utility.c utility.h 
	gcc -Wall -g -c utility.c -o utility.o

cmd_client.o: cmd_client.c cmd_client.h
	gcc -Wall -g -c cmd_client.c -o cmd_client.o

gioco.o: gioco.c gioco.h
	gcc -Wall -g -c gioco.c -o gioco.o




clean:
	rm *o client server other