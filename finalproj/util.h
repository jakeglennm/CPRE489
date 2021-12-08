
#define control_port 9897
#define data_port1 9898
#define data_port2 9899
#define data_port3 10001
#define MSGSIZE 1000

struct Set{
	int gblseq;
	int subseq;
	int sub;
	char data[5];
};
typedef struct Set Set;

/* initializing pipes and sockets */
void initPipes(int fd1[2], int fd2[2], int fd3[2]);
void initSocket(int* sckt);
void setBindAndListen(int* sckt, struct sockaddr_in* addr, int size, char* msg);

/* establishing connections */
void connectSocket(int* sckt, struct sockaddr_in* addr, int size, char* msg);
void formServerAddress(struct sockaddr_in* addr, int port);
void formClientAddress(struct sockaddr_in* addr, char* ip, int port);
void acceptConnection(int* listeningSckt, int* acceptingSckt, struct sockaddr_in* addr, int* len, char* msg);

/* inserting data from source char array to destination char array */
void insertData(char* dest, int insertIdx, char* source, int readIdx, int size);

/* managing the log */
void insertLogEntry(Set* logData, int gblseq, int subseq, int sub);
void printLog(Set* logData, int size, char* bytes);

/* helper fun'n for printing a char buffer */
void printBuffer(char* buffer) ;
