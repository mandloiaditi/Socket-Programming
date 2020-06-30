#include <arpa/inet.h> 
#include <errno.h> 
#include <netinet/in.h> 
#include <signal.h> 
#include <stdio.h> 
#include <stdlib.h> 
#include <string.h> 
#include <sys/socket.h> 
#include <sys/types.h> 
#include <unistd.h> 
#include <time.h>
#include "packet.h"
#include <sys/time.h>

void dieError(char * s){
		perror(s);
		exit(1);
}

// DELAYS IMPLEMENTED FOR EACH PACKET SENT (i.e. per packet basis)
typedef struct timer_data
{
    clock_t start_time;
    DATA_PKT  pkt;
    int sfd;
    double delay;
    struct sockaddr_in* send ;
}timer_data;
char * get_timestamp();

int main(int argc, char const *argv[])
{
	int server = 0;/* code */
	int recv_len,slen;
    if(argv[1]== NULL){
        printf("PROVIDE ARGUMENT FOR RELAY NODE\n");
        exit(1);
    }
	DATA_PKT rcv_packet;
	int isFilled[WINDOW_SIZE] = {0};
    timer_data delays[WINDOW_SIZE];
	struct sockaddr_in serv_addr, client_addr, s_in,s3;
    char node[10] = {'\0'};
	server = socket(AF_INET, SOCK_DGRAM, IPPROTO_UDP);
	memset(&serv_addr, 0, sizeof(serv_addr));
    int len = sizeof(s_in);
	serv_addr.sin_family = AF_INET;
    serv_addr.sin_addr.s_addr = htonl(INADDR_ANY);
    if(argv[1]!=NULL && (strcmp(argv[1],"0") ==0 )){
        serv_addr.sin_port = htons(7003); // port
        strcpy(node,"RELAY1");
    }
    else{
        serv_addr.sin_port = htons(7004);
        strcpy(node,"RELAY2");
    }
    
    unsigned long int lastReceived;

    s3.sin_family = AF_INET;
    s3.sin_port = htons(7005); 
    s3.sin_addr.s_addr = inet_addr("127.0.0.1");

    //bind socket to port
    if( bind(server, (struct sockaddr*) &serv_addr, sizeof(serv_addr) ) == -1)
    {
        dieError("bind");
    }

    // making recvfrom non blocking
    struct timeval non_block;
    non_block.tv_sec = 0;
    non_block.tv_usec = 0;
    setsockopt(server, SOL_SOCKET, SO_RCVTIMEO, (const char*)&non_block, sizeof non_block);
    setsockopt(server, SOL_SOCKET, SO_SNDTIMEO, (const char*)&non_block,  sizeof non_block);
    int drop = 0;
    printf("%-12s%-12s%-25s%-12s%-12s%-12s%-12s\n","Name","EventType","Timestamp","PacketTy.","Seq.No","Source","Dest");
    while(1){

    	if((recv_len = recvfrom(server, &rcv_packet, sizeof(rcv_packet), 0, (struct sockaddr*)&s_in, &len)) > 0){

            if (rand() % 100 > PDR)
                drop = 0; //drop=1 data pkt lost/dropped
            else    
                drop = 1; 
        if(rcv_packet.hd.sq_no == -99){
            break;
        }
    	else if(rcv_packet.hd.isACK == 0 && drop == 0 ){
            //save the address of client ;
            client_addr = s_in;
    		

            float millisec = rand()/(double)RAND_MAX * 2 ;
            int usecs = millisec * 1000;

                printf("%-12s%-12s%-25s%-12s%-12d%-12s%-12s\n",node,"R",get_timestamp(),"DATA",rcv_packet.hd.sq_no,"CLIENT",node); 
                usleep(usecs);

                if (sendto(server, &rcv_packet, sizeof(rcv_packet), 0, (struct sockaddr*)&s3, sizeof(s3)) == -1)
                {
                    dieError("sendto()");
                }
                printf("%-12s%-12s%-25s%-12s%-12d%-12s%-12s\n",node,"S",get_timestamp(),"DATA",rcv_packet.hd.sq_no,node,"SERVER");
       
 
    	}
        else if(rcv_packet.hd.isACK == 1){

            printf("%-12s%-12s%-25s%-12s%-12d%-12s%-12s\n",node,"R",get_timestamp(),"ACK",rcv_packet.hd.sq_no,"SERVER",node); 
            if (sendto(server, &rcv_packet, sizeof(rcv_packet), 0, (struct sockaddr*)&client_addr, sizeof(client_addr)) == -1)
            {
                dieError("sendto()");
            }

            printf("%-12s%-12s%-25s%-12s%-12d%-12s%-12s\n",node,"S",get_timestamp(),"ACK",rcv_packet.hd.sq_no,node,"CLIENT");
        }
        else{
           printf("%-12s%-12s%-25s%-12s%-12d%-12s%-12s\n",node,"R",get_timestamp(),"DATA",rcv_packet.hd.sq_no,"CLIENT",node);
           printf("%-12s%-12s%-25s%-12s%-12d%-12s%-12s\n",node,"DROP",get_timestamp(),"DATA",rcv_packet.hd.sq_no,node,node);
        }

        }



    }
    
close(server);
printf("FILE TRANSFER COMPLETE\n");
	return 0;
}

char * get_timestamp(){
    time_t ts;
    ts = time(NULL);
    struct timeval millisec;
    gettimeofday(&millisec,NULL);
    char * timestamp = (char*)malloc(sizeof(char*)*25);
    strftime(timestamp,25,"%H:%M:%S",localtime(&ts));
    char milli[10];
    sprintf(milli,".%06ld", millisec.tv_usec);
    strcat(timestamp,milli);
    return timestamp;
}