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

//NOTE : FILE NEED TO BE COMPILED USING -lrt flag to link all requirements from <time.h>
// gcc -o client client.c -lrt



// TIMERS IMPLEMENTED FOR EACH PACKET SENT (i.e. per packet basis)
typedef struct timer_data
{
    timer_t id;
    DATA_PKT  pkt;
    int sfd;
    struct sockaddr_in* send ;
}timer_data;


// For defining interval timeout for packets using TIME_OUT macro
struct itimerspec specs;  // to set timeout for all the timers
struct sockaddr_in s1,s2, s_in;
FILE *write_log ;
// KEEP track of acks received
typedef struct ackrec{
    int ack;
    struct ackrec * next;
}acks;

char * get_timestamp();
timer_data * printing ;
int printflag = 0; //printf is not a safe function to be called from signal handler



/*
Signal handler for timeout. packet with respect to which timeout has occurred is stored into a 
pointer in siginfo_t pointer si and only the corresponding packet is resend.
*/
static void handle_alarm( int sig, siginfo_t *si, void *u ){
	timer_data * data = si->si_value.sival_ptr;
	//can't use printf from a signal handler , it is not safe // will handle this through print flag
	sendto(data->sfd, &(data->pkt), sizeof(data->pkt), 0, (struct sockaddr * )(data->send), sizeof(*(data->send)));
	printing = data;
    printflag = 1;
	return;
}


/*
Initialise timer (multiple timers can be initialised each identified later with unique timer_t id 
field in timer_data struct.
*/
static int initializeTimer(timer_data* data) 
{ 
        struct sigevent se; 
        struct sigaction  sa; 

        /* Set up signal handler. */
        sa.sa_sigaction = handle_alarm;  
        sa.sa_flags = SA_SIGINFO;  
        sigemptyset(&sa.sa_mask); 

        if (sigaction(SIGRTMIN, &sa, NULL) == -1) { 
            printf("sigaction error\n");
            return -1; 
        } 
        /* Set and enable alarm */         
        se.sigev_signo = SIGRTMIN;
        se.sigev_notify = SIGEV_SIGNAL; 
        se.sigev_value.sival_ptr = data;
    	timer_create(CLOCK_REALTIME, &se, &(data->id));
        timer_settime(data->id, 0, &specs, NULL); 

        return 0; 
}



void dieError(char * s){
		perror(s);
		exit(1);
}

DATA_PKT create_packet(int size, int sequence, int offset,int isLast, int isACK );
void  insertInOrder(acks ** head, acks * new) ;
char * get_timestamp();
int main(){
	printflag  =0 ;
	// creating timer requirements:
	//convert into appropriate value
    int sec1 = TIME_OUT;
    long nanosec = (TIME_OUT-sec1)*1000000000;
    specs.it_value.tv_sec = sec1;
    specs.it_value.tv_nsec = nanosec; //(sec-sec1) * 1000000000;
    specs.it_interval.tv_sec = sec1;
    specs.it_interval.tv_nsec =nanosec; //sec-sec1) * 1000000000;
    //specs is a global variable used everywhere to set same TIME_OUT for each packet
    //timer requirements ended

	
	int lastUnAcked = 0;
	DATA_PKT sent[WINDOW_SIZE];
	int isFilled[WINDOW_SIZE];
	for(int i = 0 ;i <WINDOW_SIZE ; i++){
		isFilled[i] =0;
	}
	acks * acKRcvd  = NULL;
	// signal(SIGALRM, handle_alarm);
	timer_data timers[WINDOW_SIZE];
	
	int socketd, i ;
	int offset = 0;
	DATA_PKT send_pkt1,send_pkt2,rcv_ack1,rcv_ack2;
	// create sockets to connect to relays
	if((socketd  = socket(AF_INET,SOCK_DGRAM,IPPROTO_UDP))==-1){
		dieError("socket");
	}
	//open  file from which data is to be uploaded

	FILE * fp ;
	fp = fopen("input.txt","rb");
	if(fp == NULL){
		printf("Error Opening File\n");
		return(1);
	}
	
	int len = sizeof(s_in);

	s1.sin_family = AF_INET;
    s1.sin_port = htons(7003); // port
    s1.sin_addr.s_addr = inet_addr("127.0.0.1");

	s2.sin_family = AF_INET;
    s2.sin_port = htons(7004); // port
    s2.sin_addr.s_addr = inet_addr("127.0.0.1");

     int next_sq_no = 0 ;
    int nread=0;
    int isLast = 0;

    fd_set rset;
    int max_sd, recv_len;

    struct timeval non_block;
	non_block.tv_sec = 0;
	non_block.tv_usec = 0;

    // Make recvfrom non blocking
    setsockopt(socketd, SOL_SOCKET, SO_RCVTIMEO, (const char*)&non_block, sizeof non_block);
    setsockopt(socketd, SOL_SOCKET, SO_SNDTIMEO, (const char*)&non_block,  sizeof non_block);
    //Node
    printf("%-12s%-12s%-25s%-12s%-12s%-12s%-12s\n","Name","EventType","Timestamp","PacketTy.","Seq.No","Source","Dest");
    while(1){

    	// clear the sockets
    	if(printflag ==1){
		    char dest[10] = {'\0'};
			if(printing->send->sin_port == s1.sin_port){
				strcpy(dest,"RELAY1");
			}
			else{
				strcpy(dest,"RELAY2");
			}
    		printf("%-12s%-12s%-25s%-12s%-12d%-12s%-12s\n","CLIENT","TO",get_timestamp(),"ACK",printing->pkt.hd.sq_no,"CLIENT",dest);
    		printf("%-12s%-12s%-25s%-12s%-12d%-12s%-12s\n","CLIENT","RE",get_timestamp(),"DATA",printing->pkt.hd.sq_no,"CLIENT",dest);
    		printflag = 0;
    	}
    	char buff[PACKET_SIZE] ={'\0'};
    	// FD_ZERO(&rset);
    	non_block;
    	non_block.tv_sec = 0;
    	non_block.tv_usec = 0;

    	if((next_sq_no - lastUnAcked + 1 <= WINDOW_SIZE) && isLast == 0){

	    	offset = ftell(fp);
	                    
	        nread = fread(buff,1,PACKET_SIZE,fp);

	        if (nread < PACKET_SIZE ){
	            isLast = 1;
	        }
	        if(nread <0){
	        	break;
	        }
	        send_pkt1 = create_packet(nread,next_sq_no,offset,isLast,0);
	        memcpy(send_pkt1.data,buff,nread);

	        if(next_sq_no % 2 == 0){
		    	if (sendto(socketd, &send_pkt1, sizeof(send_pkt1), 0, (struct sockaddr*)&s1, sizeof(s1)) == -1)
		        {
		            dieError("sendto()");
		        }
		        printf("%-12s%-12s%-25s%-12s%-12d%-12s%-12s\n","CLIENT","S",get_timestamp(),"DATA",send_pkt1.hd.sq_no,"CLIENT","RELAY1");
		        timer_data new ;
		        new.pkt = send_pkt1 ;
		        new.sfd = socketd;
		        new.send  = &s1;
		        
	    		for(int i =0 ; i < WINDOW_SIZE ; i++){
	    			if( isFilled[i] == 0){
	    				sent[i] = send_pkt1;
	    				timers[i] = new;
	    				initializeTimer(&timers[i]); 
	    				isFilled[i] =1;
	    				break;
	    			}
	    		}   
	    	}
	    	else{

	    		if (sendto(socketd, &send_pkt1, sizeof(send_pkt1), 0, (struct sockaddr*)&s2, sizeof(s2)) == -1)
		        {
		            dieError("sendto()");
		        }
		        printf("%-12s%-12s%-25s%-12s%-12d%-12s%-12s\n","CLIENT","S",get_timestamp(),"DATA",send_pkt1.hd.sq_no,"CLIENT","RELAY2"); 
		        timer_data new ;
		        new.pkt = send_pkt1 ;
		        new.sfd = socketd;
		        new.send  = &s2;
		        
		        for(int i =0 ; i < WINDOW_SIZE ; i++){
	    			if( isFilled[i] == 0){
	    				sent[i] = send_pkt1;
	    				timers[i] = new;
	    				initializeTimer(&timers[i]); 
	    				isFilled[i] =1;
	    				break;
	    			}
	    		}   
	    		if(i == WINDOW_SIZE)
	    		printf("PROBLEM\n" );

	    	}

	    	next_sq_no ++;
    	}

    	if((recv_len = recvfrom(socketd, &rcv_ack1, sizeof(rcv_ack1), 0, (struct sockaddr*)&s_in, &len)) > 0){
    		char source[10] ={'\0'};
    		if(s_in.sin_port == s1.sin_port){
    			strcpy(source,"RELAY1");
    		}
    		else{
    			strcpy(source,"RELAY2");
    		}
    		printf("%-12s%-12s%-25s%-12s%-12d%-12s%-12s\n","CLIENT","R",get_timestamp(),"ACK",rcv_ack1.hd.sq_no,source,"CLIENT"); 
	    	if(rcv_ack1.hd.sq_no == lastUnAcked){

	    		for(int i =0 ; i < WINDOW_SIZE ; i++){
	    			if( isFilled[i] == 1 && sent[i].hd.sq_no == lastUnAcked){
	    				isFilled[i] = 0;
	    				timer_delete(timers[i].id);
	    				break;
	    			}
    			}  
	    		lastUnAcked ++;

	    		while(acKRcvd != NULL && lastUnAcked == acKRcvd->ack){
	    			for(int i =0 ; i < WINDOW_SIZE ; i++){
		    			if( isFilled[i] == 1 && sent[i].hd.sq_no == lastUnAcked){
		    				isFilled[i] = 0;
		    				break;
		    			}
    				}  
	    			lastUnAcked ++;
                    acKRcvd =  acKRcvd->next;
                }	
	    	}
	    	else{
		    		acks * new  = (acks *)malloc(sizeof(acks));
	                new->ack = rcv_ack1.hd.sq_no;
	                new->next = NULL;

	                for(int i =0 ; i < WINDOW_SIZE ; i++){
		    			if( isFilled[i] == 1 && sent[i].hd.sq_no == rcv_ack1.hd.sq_no){
		    				timer_delete(timers[i].id);
		    				break;
		    			}
	    			}  
                	insertInOrder(&acKRcvd, new);
	    	}

    	}
   	 if(acKRcvd == NULL && isLast == 1 &&(lastUnAcked == next_sq_no)){
   	 	break;
   	 }
    
    }
close(socketd);
fclose(fp);
printf("FILE TRANSFER COMPLETE\n");
}



DATA_PKT create_packet(int size, int sequence, int offset,int isLast, int isACK ){
	DATA_PKT * new = (DATA_PKT *)malloc(sizeof(DATA_PKT));
	new->hd.isACK = isACK;
	new->hd.size = size;
	new->hd.offset = offset;
	new->hd.sq_no = sequence;
	new->hd.isLast = isLast;
	memset( new->data, '\0', PACKET_SIZE );
	return *new;
}

void  insertInOrder(acks ** head, acks * new)  
{      acks * cur;  
    if (  *head == NULL|| (*head)->ack > new->ack  )  
    {  
        new->next = *head;  
        *head = new;  
    } 
    else if( (*head)->ack == new->ack)
    return ; 
    else
    {  
        cur = *head;  
        while (cur->next!=NULL && cur->next->ack < new->ack)  
            cur = cur->next; 
        if(cur->ack == new->ack){
            return ;
        } 
        new->next = cur->next;  
        cur->next = new;  
    } 
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
