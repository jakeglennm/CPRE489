#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <string.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include "util.h"
 
struct sockaddr_in DSS_ServAddr, Control_Addr, Client_Addr1, Client_Addr2, Client_Addr3, Data_Addr1, Data_Addr2, Data_Addr3;
int controlfd, acceptfd, acceptfd1, acceptfd2, acceptfd3,datafd1, datafd2, datafd3, ControlLen, ClientLen1, ClientLen2, ClientLen3;
int parent,pid1, pid2, pid3;
int cp1[2], cp2[2], cp3[2], pc1[2], pc2[2], pc3[2];

int main(int argc, char** argv)
{
	/* init variables */
	char mainBuffer[1000], buffer1[248], buffer2[248], buffer3[248];
	Set logData[248];
	int subSeqNum1, subSeqNum2, subSeqNum3;

	subSeqNum1 = 0; // counting starts at 0
	subSeqNum2 = 0;
	subSeqNum3 = 0;

	parent = getpid();

	/* initalize pipes for communication b/w parent and children */
	initPipes(cp1, cp2, cp3); // child to parent
	initPipes(pc1, pc2, pc3); // parent to child

	/* initalize sockets for the control and 3 subflow connections */
	initSocket(&controlfd);
	initSocket(&datafd1);
	initSocket(&datafd2);
	initSocket(&datafd3);
 
	/* form addresses for the control and 3 subflow connections */
	formServerAddress(&DSS_ServAddr, control_port);
	formServerAddress(&Data_Addr1, data_port1);
	formServerAddress(&Data_Addr2, data_port2);
	formServerAddress(&Data_Addr3, data_port3);

 
	/* bind and listen on all sockets */
	setBindAndListen(&controlfd, &DSS_ServAddr, sizeof(DSS_ServAddr), "control bind/listen failed"); 
	setBindAndListen(&datafd1, &Data_Addr1, sizeof(Data_Addr1), "subflow1 bind/listen failed"); 
	setBindAndListen(&datafd2, &Data_Addr2, sizeof(Data_Addr2), "subflow2 bind/listen failed"); 
	setBindAndListen(&datafd3, &Data_Addr3, sizeof(Data_Addr3), "subflow3 bind/listen failed"); 

	/* accept all connections (blocking) */
	acceptConnection(&controlfd, &acceptfd, &DSS_ServAddr, &ControlLen, "control accept failed");
	acceptConnection(&datafd1, &acceptfd1, &Data_Addr1, &ClientLen1, "subflow1 accept failed");
	acceptConnection(&datafd2, &acceptfd2, &Data_Addr2, &ClientLen2, "subflow2 accept failed");
	acceptConnection(&datafd3, &acceptfd3, &Data_Addr3, &ClientLen3, "subflow3 accept failed");


	/* fork child processes 1,2,3 */
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

	/* recieve data */
 
	for(;;)
	{
		char msg[MSGSIZE];
		int currentProcess = getpid();

		if(currentProcess == parent) // parent
		{
			char rcvDSS[MSGSIZE];
			char reqData[MSGSIZE];
			char recData[MSGSIZE];
			int mappedSeq, subSeq, subIdx, mainIdx;
			

			/* get gbl sequence # from dss connection, map it to a sub sequence  #
			then translate each to an index within the parent & child buffers */
			if(read(acceptfd, rcvDSS, MSGSIZE) < 0)
			{
				perror("read dss failed");
				exit(1);
			}
			mappedSeq = atoi(rcvDSS);
			subSeq = mappedSeq / 3;
			mainIdx = mappedSeq*4; // start index of where the data belongs in the buffer
			subIdx = subSeq*4; // start index of where the child reads data from
			sprintf(reqData, "%d", subIdx);

			/* map to a subflow then request data from child and recieve it */
			if(mappedSeq%3 == 0) // subflow 1
			{
				write(pc1[1], reqData, MSGSIZE); 
				if(read(cp1[0], recData, MSGSIZE) < 0)
				{
					perror("read data failed");
					exit(1);
				}
			}
			else if(mappedSeq%3 == 1) // subflow 2
			{
				write(pc2[1], reqData, MSGSIZE); 
				if(read(cp2[0], recData, MSGSIZE) < 0)
				{
					perror("read data failed");
					exit(1);
				}
			}
			else if(mappedSeq%3 == 2) // subflow3
			{	
				write(pc3[1], reqData, MSGSIZE); 
				if(read(cp3[0], recData, MSGSIZE) < 0)
				{
					perror("read data failed");
					exit(1);
				}
			}

			/* insert data into main buffer + log mapping */
			insertData(mainBuffer, mainIdx, recData, 0, 4);
			insertLogEntry(logData, mappedSeq, subSeq, (mappedSeq%3+1));

			if(mappedSeq >= 247)
			{ 
				mainBuffer[992] = '\0';
				break;
			}
		}
		else if(currentProcess == pid1) // child 1 - subflow 1
		{
			if(subSeqNum1 >= 83) break;
	
			char rcvData[1000];
			char readReq[MSGSIZE];
			char sendData[MSGSIZE];

			/* read data from client */
			if(read(acceptfd1, rcvData, 1000) < 0)
			{
				perror("read 1 failed");
				exit(1);
			}

			/* insert data to local buffer */
			int insertIdx = subSeqNum1 * 4;
			insertData(buffer1, insertIdx, rcvData, 0, 4);

			/* read request for data from parent then send it back */
			if(read(pc1[0], readReq, 1000) < 0)
			{
				perror("read 1 failed");
				exit(1);
			}
			int readIdx = atoi(readReq);
			insertData(sendData, 0, buffer1, readIdx, 4);
			sendData[5] = '\0';
			write(cp1[1], sendData, MSGSIZE);

			subSeqNum1++; // indicate new packet was sent to server
		} 
		else if(currentProcess == pid2)  // child 2 - subflow 2
		{
			if(subSeqNum2 >= 83) break;
				
			char rcvData[1000];
			char readReq[MSGSIZE];
			char sendData[MSGSIZE];

			/* read data from client */
			if(read(acceptfd2, rcvData, 1000) < 0)
			{
				perror("read 2 failed");
				exit(1);
			}

			/* insert data to local buffer */
			int insertIdx = subSeqNum2 * 4;
			insertData(buffer2, insertIdx, rcvData, 0, 4);

			/* read request for data from parent then send it back */
			if(read(pc2[0], readReq, 1000) < 0)
			{
				perror("read 2 failed");
				exit(1);
			}
			int readIdx = atoi(readReq);
			insertData(sendData, 0, buffer2, readIdx, 4);
			sendData[5] = '\0';
			write(cp2[1], sendData, MSGSIZE);

			subSeqNum2++; // indicate new packet was sent to server

		} 
		else if(currentProcess == pid3)  // child 3 - subflow 3
		{
			if(subSeqNum3 >= 82) break;
	
			char rcvData[1000];
			char readReq[MSGSIZE];
			char sendData[MSGSIZE];

			/* read data from client */
			if(read(acceptfd3, rcvData, 1000) < 0)
			{
				perror("read 3 failed");
				exit(1);
			}

			/* insert data to local buffer */
			int insertIdx = subSeqNum3 * 4;
			insertData(buffer3, insertIdx, rcvData, 0, 4);

			/* read request for data from parent then send it back */
			if(read(pc3[0], readReq, 1000) < 0)
			{
				perror("read 3 failed");
				exit(1);
			}
			int readIdx = atoi(readReq);
			insertData(sendData, 0, buffer3, readIdx, 4);
			sendData[5] = '\0';
			write(cp3[1], sendData, MSGSIZE);
		
			subSeqNum3++; // indicate new packet was sent to server
		} 

	}

	/* print final buffer and log */
	if(getpid() == parent)
	{
		printf("Server Side - Sequence Number Log\n\n");
		printLog(logData, 248, mainBuffer);
 		printf("\nMessage Recieved\n\n%s\n\n", mainBuffer);
	}

	close(controlfd);
	close(datafd1);
	close(datafd2);
	close(datafd3);
	
	return 0;
 
}
