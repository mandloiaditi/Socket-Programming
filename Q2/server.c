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
#include <sys/time.h>
#include "packet.h"

void dieError(char * s){
		perror(s);
		exit(1);
}


// KEEP track of out of order packets into char buffer received
// maximum such char buffers that can be used is defined as MAX_BUFF_SIZE
// After reaching this limit , subsequent out of order packets will be dropped 
typedef struct out{
    int next_in_order; // for ordering for write
    char buff[PACKET_SIZE]; // only char array stored not whole packet
    struct out * next;
}out_of_order;

char * get_timestamp();

int  insertInOrder(out_of_order * new,out_of_order ** head)  ;
int main(int argc, char const *argv[])
{
	int server = 0;/* code */
	int recv_len,slen;
	DATA_PKT rcv_packet ,ack_pkt;

    out_of_order * buffhead = NULL;
    int lastSize = 0;


    unsigned long int receiverWindow =0 ; //base of current window
    int buffSize = 0; // to keep track if buffer is not full 


	
	struct sockaddr_in serv_addr, s_in, nodes[2];
    int len = sizeof(s_in);
	server = socket(AF_INET, SOCK_DGRAM, IPPROTO_UDP);
	memset(&serv_addr, 0, sizeof(serv_addr));

	serv_addr.sin_family = AF_INET;
    serv_addr.sin_addr.s_addr = htonl(INADDR_ANY);
    serv_addr.sin_port = htons(7005);

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
    


    FILE *fp = fopen("ouput.txt","wb");
    if(fp==NULL)
    {
        printf("Error Opening File\n");
         exit(1);   
    }      


    int  lastReceived;
    printf("%-12s%-12s%-25s%-12s%-12s%-12s%-12s\n","Name","EventType","Timestamp","PacketTy.","Seq.No","Source","Dest");
    while(1){

    	if((recv_len = recvfrom(server, &rcv_packet, sizeof(rcv_packet), 0, (struct sockaddr*)&s_in, &len)) > 0){
        char node[10] = {'\0'};
        if(s_in.sin_port == htons(7003)){
            strcpy(node,"RELAY1");
            nodes[0] = s_in;
        }
        else{
            strcpy(node,"RELAY2");
            nodes[1] = s_in;
        }

    	if(rcv_packet.hd.isACK == 0 && rcv_packet.hd.sq_no >= receiverWindow){
    		printf("%-12s%-12s%-25s%-12s%-12d%-12s%-12s\n","SERVER","R",get_timestamp(),"DATA",rcv_packet.hd.sq_no,node,"SERVER");

            ack_pkt.hd.sq_no = rcv_packet.hd.sq_no;
            ack_pkt.hd.isACK =1;
            
            //writing to file
            if(rcv_packet.hd.sq_no == receiverWindow){
                if(rcv_packet.hd.isLast ==1){
                    lastReceived =1;
                    lastSize = rcv_packet.hd.size;
                }
                fwrite(rcv_packet.data,1,rcv_packet.hd.size,fp);
                receiverWindow ++;
                while(buffhead != NULL && buffhead->next_in_order == receiverWindow){
                    if(buffhead->buff[PACKET_SIZE-1] !=EOF)
                    fwrite(buffhead->buff,1,PACKET_SIZE,fp);
                    else{
                        fwrite(buffhead->buff,1,lastSize,fp);
                    }
                    receiverWindow ++;
                    buffSize --; //decrement as buffer space is freed;
                    buffhead =  buffhead->next;
                }
                if (sendto(server, &ack_pkt, sizeof(ack_pkt), 0, (struct sockaddr*)&s_in, sizeof(s_in)) == -1)
                {
                    dieError("sendto()");
                }


                printf("%-12s%-12s%-25s%-12s%-12d%-12s%-12s\n","SERVER","S",get_timestamp(),"ACK",ack_pkt.hd.sq_no,"SERVER",node);
            }
            else{
                if(buffSize < MAX_BUFF_SIZE){
                    out_of_order * new  = (out_of_order *)malloc(sizeof(out_of_order));
                    new->next_in_order = rcv_packet.hd.sq_no;
                    new->next = NULL;
                    memset(new->buff,EOF,PACKET_SIZE);
                    memcpy(new->buff,rcv_packet.data,rcv_packet.hd.size);
                    if(rcv_packet.hd.isLast ==1){
                        lastReceived =1;
                        lastSize = rcv_packet.hd.size;
                    }
                    int inserted = insertInOrder( new,&buffhead);
                    buffSize ++;
                    if(inserted ==1){
                        // only send ack if the data is written over file or successfully buffered
                        if (sendto(server, &ack_pkt, sizeof(ack_pkt), 0, (struct sockaddr*)&s_in, sizeof(s_in)) == -1)
                        {
                            dieError("sendto()");
                        }
                        printf("%-12s%-12s%-25s%-12s%-12d%-12s%-12s\n","SERVER","S",get_timestamp(),"ACK",ack_pkt.hd.sq_no,"SERVER",node);
                    }
                }
                else{
                    // buffer was full can not buffer the packet 
                    // hence packet is dropped
                    // no ack sent in this case;
                    // write write_log for packet dropped at server
                    printf("%-12s%-12s%-25s%-12s%-12d%-12s%-12s\n","SERVER","D(BUFFER FULL)",get_timestamp(),"DATA",rcv_packet.hd.sq_no,"SERVER","SERVER");
                }
            }

    	}

        if(lastReceived ==1 && buffhead == NULL){
            break;
        }
        }



    }
    ack_pkt.hd.sq_no = -99;

    //send file tansfer message to the relay nodes.

    if (sendto(server, &ack_pkt, sizeof(ack_pkt), 0, (struct sockaddr*)&(nodes[0]), sizeof(nodes[0])) == -1)
    {
        dieError("sendto()");
    }


    if (sendto(server, &ack_pkt, sizeof(ack_pkt), 0, (struct sockaddr*)&nodes[1], sizeof(nodes[1])) == -1)
    {
        dieError("sendto()");
    }

    fclose(fp);
    close(server);
    printf("FILE TRANSFER COMPLETE\n");
	return 0;
}




int  insertInOrder(out_of_order * new,out_of_order ** head)  
{      out_of_order * cur;  
    if ( *head == NULL|| (*head)->next_in_order > new->next_in_order )  
    {  
        new->next = *head;  
        *head = new;  
    }
    else if((*head)->next_in_order == new->next_in_order ){
        return 0 ;
    }  
    else
    {  
        cur = *head;  
        while (cur->next!=NULL && cur->next->next_in_order < new->next_in_order)  
            cur = cur->next;
        if(cur->next_in_order == new->next_in_order){
            return 0;
        }  
        new->next = cur->next;  
        cur->next = new;  
    } 
    return 1;
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