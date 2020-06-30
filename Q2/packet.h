
#include<stdio.h>
#include<string.h>
#include<time.h>
#include <sys/time.h>
#define PACKET_SIZE 100
#define PDR 10  //percent of packet drop

#define WINDOW_SIZE 8
#define TIME_OUT  2 //in seconds // should sufficiently large to bypass worst case delay


#define MAX_BUFF_SIZE 20 
// can be changed file should be around ten times larger than this
// We try to keep it to be more or equal to order of window size



typedef struct head{
	int size;
	int sq_no;
	int offset;
	int isLast;
	int isACK;
}header;


typedef struct data_pkt{
	header hd;
	char data[PACKET_SIZE];
}DATA_PKT;


