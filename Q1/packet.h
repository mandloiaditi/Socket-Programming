
#include<stdio.h>
#include<string.h>
#include<time.h>

#define PACKET_SIZE 100
#define TIME_OUT 2 // in seconds
#define PDR 10 // packet drop rate in percent


typedef struct head{
	int size;
	int sq_no;
	int isLast;
	int isACK;
	int channel_num;
}header;


typedef struct data_pkt{
	header hd;
	char data[PACKET_SIZE];
}DATA_PKT;
