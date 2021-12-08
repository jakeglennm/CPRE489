all: server client

server: util.c server.c
	gcc -o server server.c util.c
client: util.c client.c
	gcc -o client client.c util.c


clean: 
	rm client server 
