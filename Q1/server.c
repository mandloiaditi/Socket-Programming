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
#include "packet.h"


// Used only for out of order (very less buffering)
// Data written in file as soon as in order packets arrive
typedef struct buff{
    DATA_PKT data;
    struct buff * next;
}buffer;

void  insertInOrder(buffer * * root, buffer * new)  
{  
    buffer * curr;  
    if (*root == NULL || (*root)->data.hd.sq_no > new->data.hd.sq_no)  
    {  
        new->next = *root;  
        *root = new;  
    }  
    else
    {  
        curr = *root;  
        while (curr->next!=NULL && curr->next->data.hd.sq_no < new->data.hd.sq_no)  
            curr = curr->next;
        if(curr->data.hd.sq_no == new->data.hd.sq_no){
            return ;
        }    
        new->next = curr->next;  
        curr->next = new;  
    }  
} 

int main(){
	int listenfd, connfd[2],bytesReceived,confd;
	struct sockaddr_in clientaddr, serveraddr;
	fd_set rset;
	buffer * buffhead = NULL;
	DATA_PKT rcv_packet, ack_pkt;
	serveraddr.sin_family = AF_INET;
    serveraddr.sin_addr.s_addr = htonl(INADDR_ANY);
    serveraddr.sin_port = htons(5001);

    int slen = sizeof(serveraddr);
	int max_sd;
    int offset = 0;
    int drop = 0;
    int opt =1;
    /* create a listening TCP socket (MASTER SOCKET) */

	listenfd = socket(AF_INET,SOCK_STREAM,0);

    for (int i = 0; i < 2; i++){
        connfd[i] = 0;
    }

    FILE *fp = fopen("ouput.txt","wb");
    if(fp==NULL)
    {
        printf("Error Opening File\n");
        return 1;   
    }        

    // Allow master socket for multiple connections//
    if( setsockopt(listenfd, SOL_SOCKET, SO_REUSEADDR, (char *)&opt, sizeof(opt)) < 0 ){
        perror("setsockopt");
        exit(EXIT_FAILURE);
    }
        
    bind(listenfd, (struct sockaddr*)&serveraddr,sizeof(serveraddr));

    if(listen(listenfd, 10) == -1)
    {
        printf("Failed to listen\n");
        return -1;
    }
    int brk = 0;
    int isLast =0;
    int clientLength = sizeof(clientaddr);
    connfd[0] = accept(listenfd, (struct sockaddr*)&clientaddr, &clientLength);
    connfd[1] = accept(listenfd, (struct sockaddr*)&clientaddr, &clientLength);
    int i =0;
    while(1){
            drop = 0;

            //clear the socket set  
            FD_ZERO(&rset);   
         
            //add master socket to set  
            FD_SET(listenfd, &rset);   
            max_sd = listenfd;  

            for (int  i = 0 ; i < 2 ; i++)   
            {   
                int sd = connfd[i];   
                if(sd > 0)   
                    FD_SET( sd , &rset);   
                if(sd > max_sd)   
                    max_sd = sd;   
            } 

            struct timeval non_block;
            non_block.tv_sec = 0;
            non_block.tv_usec = 0;
            int activity = select( max_sd + 1 , &rset , NULL , NULL , &non_block);   

            for(int i = 0 ; i < 2 ; i++){

                confd = connfd[i];   
                 
                if (FD_ISSET( confd , &rset)){
                    if (rand() % 100 > PDR)
                        drop = 0; //drop=1 data pkt lost/dropped
                    else    
                        drop = 1; 

                	if(recvfrom(confd, &rcv_packet, sizeof(rcv_packet), 0,  (struct sockaddr*)&clientaddr, &clientLength) == -1){
                		printf("Error Receiving packet\n");
                        drop =1 ;
                    }
                    else{
                    	if(drop != 1 ){
                                printf("RCVD PKT: Seq No: %d of Size: %d from channel: %d\n",rcv_packet.hd.sq_no,rcv_packet.hd.size,rcv_packet.hd.channel_num);
                                if(rcv_packet.hd.sq_no == offset){

                                    // write immediately the packet received is in order	
                                    fwrite(rcv_packet.data,1,rcv_packet.hd.size,fp);
                                    offset = rcv_packet.hd.sq_no + rcv_packet.hd.size;
                                    if(rcv_packet.hd.isLast == 1){
                                        isLast =1;
                                    }
                                    // Write all the data as soon as any missing packet is received requires very less buffering;
                                    while(buffhead != NULL && offset == buffhead->data.hd.sq_no){
                                        fwrite(buffhead->data.data,1,buffhead->data.hd.size,fp);
                                        offset = buffhead->data.hd.sq_no + buffhead->data.hd.size;
                                        buffhead =  buffhead->next;
                                    }

                                }
                                else{
                                    if(rcv_packet.hd.isLast == 1 || rcv_packet.hd.size ==0){
                                        isLast =1;
                                    }
                                    buffer * new  = (buffer *)malloc(sizeof(buffer));
                                    new->data = rcv_packet;
                                    new->next = NULL;
                                    insertInOrder(&buffhead, new);
                                }
                                

                                ack_pkt.hd.sq_no = rcv_packet.hd.sq_no;
                                ack_pkt.hd.channel_num = rcv_packet.hd.channel_num;
                                if (sendto(confd, & ack_pkt, sizeof(ack_pkt), 0,  (struct sockaddr*)&clientaddr, clientLength) == -1) {
                                    printf("Error Sending Ack;");
                                }
                                else{
                                    printf("SENT ACK for packet with seq. no. %d through channel : %d\n", ack_pkt.hd.sq_no, ack_pkt.hd.channel_num);
                                }

                        }
                        else{
                             printf("**DROPPED PKT: Seq No: %d of Size: %d from channel: %d\n",rcv_packet.hd.sq_no,rcv_packet.hd.size,rcv_packet.hd.channel_num);
                        }
                    }
                                  
                }
                if(isLast ==1 && buffhead == NULL) {
                        fclose(fp);
                        brk = 1;
                        break;
                }       
            }    

            if(brk ==  1){
                break;
            }
    }
       
    close(connfd[0]);
    close(connfd[1]);
    sleep(1);
}
