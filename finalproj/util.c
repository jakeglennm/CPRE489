#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <string.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <arpa/inet.h>
#include <netinet/in.h>
#include <sys/time.h>
#include <fcntl.h>        
#include "util.h"

void insertData(char* dest, int insertIdx, char* source, int readIdx, int size)
{
	int i;
	for(i = 0; i < size; i++)
	{
		int tempInsert = insertIdx + i;
		int tempRead = readIdx + i;
		*(dest + tempInsert) = *(source + tempRead); 
	}
}

void insertLogEntry(Set* logData, int gblseq, int subseq, int sub)
{
	Set* entry = logData + gblseq;
	entry->gblseq = gblseq;
	entry->subseq = subseq;
	entry->sub = sub;
}

void printLog(Set* logData, int size, char* bytes)
{
	char data[5];
	data[4] = '\0';
	printf("Global\tSubflow\tSubseq\tData\n");

	int i, idx;
	for(i = 0; i < size; i++)
	{
		Set* temp = logData + i;
		idx =  temp->gblseq*4;
		insertData(data, 0, bytes, idx, 4);
		printf("%d\t%d\t%d\t%s\n", temp->gblseq, temp->sub, temp->subseq, data);
	}

}

void printBuffer(char* buffer) 
{
	char* temp = buffer;
	while(*temp != '\0')
	{
		char c = *temp;
		printf("%c",c);
		temp++;
	}
	printf("\n");
}

void initPipes(int fd1a[2], int fd2a[2], int fd3a[2])
{	
	if(pipe(fd1a) < 0) exit(1);
	if(pipe(fd2a) < 0) exit(1);
	if(pipe(fd3a) < 0) exit(1);
}


void initSocket(int* sckt)
{
	if ( (*sckt = socket(AF_INET, SOCK_STREAM, 0)) < 0 ) 
	{
        	perror("socket creation failed");
        	exit(EXIT_FAILURE);
    	}

	int enable = 1;
	if (setsockopt(*sckt, SOL_SOCKET, SO_REUSEADDR, &enable, sizeof(int)) < 0)
		error("setsockopt failed");

}

void setBindAndListen(int* sckt, struct sockaddr_in* addr, int size, char* msg)
{
	if ( bind(*sckt, (const struct sockaddr *) addr, size) < 0 )
    	{
      		perror(msg);
        	exit(EXIT_FAILURE);
    	}

	if ((listen(*sckt, 5)) != 0) 
	{
		perror(msg);
		exit(EXIT_FAILURE);	
	}
	
}
void formServerAddress(struct sockaddr_in* addr, int port)
{
	addr->sin_family = AF_INET;
	addr->sin_port = htons(port);
	addr->sin_addr.s_addr = htonl(INADDR_ANY);
}

void formClientAddress(struct sockaddr_in* addr, char* ip, int port)
{
	addr->sin_family = AF_INET;
	addr->sin_port = htons(port);
	addr->sin_addr.s_addr = inet_addr(ip);
}

void connectSocket(int* sckt, struct sockaddr_in* addr, int size, char* msg)
{
	if (connect(*sckt, (struct sockaddr *) addr, size) != 0) 
	{
		perror(msg);
		exit(EXIT_FAILURE);	
	}

}

void acceptConnection(int* listeningSckt, int* acceptingSckt, struct sockaddr_in* addr, int* len, char* msg)
{
	*acceptingSckt = accept(*listeningSckt, (struct sockaddr *) addr, len); 
	if(*acceptingSckt == -1)
	{
		perror(msg);
		exit(EXIT_FAILURE);
	}
}

