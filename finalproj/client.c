#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <string.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <arpa/inet.h>
#include <netinet/in.h>
#include <sys/time.h>
#include "util.h"

#define SERVER_IP "127.0.0.1"

struct sockaddr_in DSS_Addr, DataSub1, DataSub2, DataSub3, ServSub1, ServSub2, ServSub3;
int controlfd, subflow1, subflow2, subflow3, datafd1, datafd2, datafd3;
int parent, pid1, pid2, pid3;
int cp1[2], cp2[2], cp3[2];
char bytes[992];

void generateBytes()
{
	char subMsg[62] = "0123456789abcdefghijklmnopqrstuvwxyzABCDEFGHIJKLMNOPQRSTUVWXYZ";

	int i, j;
	for(i = 0; i < 16; i++)
	{
		int base = i * 62;
		for(j = 0; j < 62; j++)
		{
			bytes[base + j] = subMsg[j];
		}
	} 
}

int main()
{
	/* init variables */
	Set logData[248];
	int subSeqNum1, subSeqNum2, subSeqNum3; // updated by child processes
	int sub1, sub2, sub3; // used by parent process to track subsequence #

	subSeqNum1 = 0; // counting starts at 0
	subSeqNum2 = 0;
	subSeqNum3 = 0;
	sub1 = 0;
	sub2 = 0;
	sub3 = 0;

	parent = getpid(); 
	generateBytes(); // fill "bytes" array

	/* initalize pipes for communication b/w parent and children */
	initPipes(cp1, cp2, cp3);

	/* initalize sockets for the control and 3 subflow connections */
	initSocket(&controlfd);
	initSocket(&subflow1);
	initSocket(&subflow2);
	initSocket(&subflow3);

	/* form addresses for the control and 3 subflow connections */
	formClientAddress(&DSS_Addr, SERVER_IP, control_port);
	formClientAddress(&DataSub1, SERVER_IP, data_port1);
	formClientAddress(&DataSub2, SERVER_IP, data_port2);
	formClientAddress(&DataSub3, SERVER_IP, data_port3);

	/* connect the control and 3 subflow sockets to the server*/
	connectSocket(&controlfd, &DSS_Addr, sizeof(DSS_Addr), "ctrl conn failed");
	connectSocket(&subflow1, &DataSub1, sizeof(DataSub1), "subflow1 conn failed");
	connectSocket(&subflow2, &DataSub2, sizeof(DataSub2), "subflow2 conn failed");
	connectSocket(&subflow3, &DataSub3, sizeof(DataSub3), "subflow3 conn failed");

	/* fork child processes 1,2,3 and store process id numbers */
	if(fork() == 0)
	{
		pid1 = getpid();
	}	
	else
	{	
		if(fork() == 0)
		{
			pid2 = getpid();
		}	
		else
		{	
			if(fork() == 0)
			{
				pid3 = getpid();
			}	
		}
	}

	/* send data */	
	for(;;)
	{
		int currentProcess = getpid();

		if(currentProcess == parent)
		{
			char recStr[1000];
			char sendStr[MSGSIZE];

			/* read from each child's pipe to get updated subsequence vaues 
			then map to global sequence number and send it to the server
			through the DSS conenction + log entry of mapping */

			if(read(cp1[0], recStr, 1000) > 0)
			{
				sub1 = atoi(recStr);
				int mappedSeq = sub1*3;
				sprintf(sendStr, "%d", mappedSeq);
				write(controlfd, sendStr, MSGSIZE);
				if(sub1 >= 0) insertLogEntry(logData, mappedSeq, sub1, 1);
			}

			if(sub2 == -1) break; // check if next subflow is done

			if(read(cp2[0], recStr, 1000) > 0)
			{
				sub2 = atoi(recStr);
				int mappedSeq = sub2*3 + 1;
				sprintf(sendStr, "%d", mappedSeq);
				write(controlfd, sendStr, MSGSIZE);
				if(sub2 >= 0) insertLogEntry(logData, mappedSeq, sub2, 2);
			}

			if(sub3 == -1) break; // check if next subflow is done

			if(read(cp3[0], recStr, 1000) > 0)
			{
				sub3 = atoi(recStr);
				int mappedSeq = sub3*3 + 2;
				sprintf(sendStr, "%d", mappedSeq);
				write(controlfd, sendStr, MSGSIZE);
				if(sub3 >= 0) insertLogEntry(logData, mappedSeq, sub3, 3);
			}

			if(sub1 == -1) break; // check if proceding subflow is done
							
		}
		else if(currentProcess == pid1)
		{
			char msg[MSGSIZE];
			char cpStr[MSGSIZE];

			/* indicate completion of task by writing -1 to parent */
			if(subSeqNum1 >= 83)
			{
				write(cp1[1], "-1", MSGSIZE);
				break;
			} 	
		
			/* extract data from "bytes" and send to server */
			int idx = subSeqNum1 * 12; // no offset
			sprintf(msg, "%c%c%c%c\0", bytes[idx], bytes[idx+1], bytes[idx+2], bytes[idx+3]);
			if(write(subflow1, msg, 1000) < 0)
			{
				perror("write 1 failed");
				exit(1);
			}

			/* write sequence number to parent */
			sprintf(cpStr, "%d", subSeqNum1);
			write(cp1[1], cpStr, MSGSIZE);
			subSeqNum1++;
		}
		else if(currentProcess == pid2)
		{

			char msg[MSGSIZE];
			char cpStr[MSGSIZE];

			/* indicate completion of task by writing -1 to parent */
			if(subSeqNum2 >= 83)
			{
				write(cp2[1], "-1", MSGSIZE);
				break;
			} 		
	
		
			/* extract data from "bytes" and send to server */
			int idx = subSeqNum2 * 12 + 4; // offset 4 bytes
			sprintf(msg, "%c%c%c%c\0", bytes[idx], bytes[idx+1], bytes[idx+2], bytes[idx+3]);
			if(write(subflow2, msg, 1000) < 0)
			{
				perror("write 2 failed");
				exit(1);
			}

			/* write sequence number to parent */
			sprintf(cpStr, "%d", subSeqNum2);
			write(cp2[1], cpStr, MSGSIZE);
			subSeqNum2++;
			
		} 
		else if(currentProcess == pid3)
		{
			char msg[MSGSIZE];
			char cpStr[MSGSIZE];

			/* indicate completion of task by writing -1 to parent */
			if(subSeqNum3 >= 82)
			{
				write(cp3[1], "-1", MSGSIZE);
				break;
			} 	

			/* extract data from "bytes" and send to server */
			int idx = subSeqNum3 * 12 + 8;
			sprintf(msg, "%c%c%c%c\0", bytes[idx], bytes[idx+1], bytes[idx+2], bytes[idx+3]);
			if(write(subflow3, msg, 1000) < 0)
			{
				perror("write 3 failed");
				exit(1);
			}

			/* write sequence number to parent */
			sprintf(cpStr, "%d", subSeqNum3);
			write(cp3[1], cpStr, MSGSIZE);
			subSeqNum3++;
		} 
		
	}

	/* print final buffer and log */
	if(getpid() == parent)
	{
		printf("Client Side - Sequence Number Log\n\n");
		printLog(logData, 248, bytes);
 		printf("\nMessage Sent\n\n%s\n\n", bytes);

	}

	close(controlfd);
	close(datafd1);
	close(datafd2);
	close(datafd3);
	return 0;
}
