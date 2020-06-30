#include <stdio.h>
#include <sys/socket.h>//for socket(), connect(), send(), recv()
#include <arpa/inet.h>// different address structures are declared
#include <stdlib.h>// atoi() which convert string to integer
#include <string.h>
#include <unistd.h> // close() function
#include <sys/time.h>
#include <signal.h>
#include "packet.h"


struct sockaddr_in serv_addr;
// struct sockaddr_in si_other;
int s, i, slen=sizeof(serv_addr);   

int rec_flg1, rec_flg2;

int sockfd1 = 0;
int sockfd2 = 0;

/* Interrupt handler to re-transmit the packet */

DATA_PKT send_pkt1,send_pkt2,rcv_ack1,rcv_ack2;

static timer_t tmid0, tmid1;

static void hndlr(int Sig, siginfo_t *Info, void *Ptr)
{
    if(Info->si_value.sival_ptr == &tmid0){
        if (rec_flg1 == 0)
        {
            sendto(sockfd1, &send_pkt1, sizeof(send_pkt1), 0, (struct sockaddr * )&serv_addr, slen);
            printf("Resending Packet for SIZE: %d SEQ NO: %d IsACK: %d IsLAST: %d CHANNEL: %d\n",send_pkt1.hd.size,send_pkt1.hd.sq_no,send_pkt1.hd.isLast,send_pkt1.hd.isACK,send_pkt1.hd.channel_num);
            //alarm(1); 
        }
    }
    else{

        if (rec_flg2 == 0)
        {
            sendto(sockfd2, &send_pkt2, sizeof(send_pkt2), 0, (struct sockaddr * )&serv_addr, slen);
            printf("Resending Packet for SIZE: %d SEQ NO: %d IsACK: %d IsLAST: %d CHANNEL: %d\n",send_pkt2.hd.size,send_pkt2.hd.sq_no,send_pkt2.hd.isLast,send_pkt2.hd.isACK,send_pkt2.hd.channel_num);
            //alarm(0.01); 
        }

    }
}



void handle_alarm(int sig) {

    if (rec_flg1 == 0)
    {
        sendto(sockfd1, &send_pkt1, sizeof(send_pkt1), 0, (struct sockaddr * )&serv_addr, slen);
        printf("Resending Packet for SIZE: %d SEQ NO: %d IsACK: %d IsLAST: %d CHANNEL: %d\n",send_pkt1.hd.size,send_pkt1.hd.sq_no,send_pkt1.hd.isLast,send_pkt1.hd.isACK,send_pkt1.hd.channel_num);
        //alarm(1); 
    }
}



DATA_PKT create_packet(int size, int sequence, int isLast, int isACK ,int channel_num){
    DATA_PKT * new = (DATA_PKT *)malloc(sizeof(DATA_PKT));
    new->hd.isACK = isACK;
    new->hd.size = size;
    new->hd.sq_no = sequence;
    new->hd.channel_num = channel_num;
    new->hd.isLast = isLast;
    return *new;
}



int main(void){
    int byteRec;


    char buff[PACKET_SIZE];
    char message[PACKET_SIZE];
    int offset1 =0,offset2 ;
    rec_flg1 =1;
    rec_flg2 = 1;
    fd_set rset;
    memset(buff, '0', sizeof(buff));


    serv_addr.sin_family = AF_INET;
    serv_addr.sin_port = htons(5001); // port
    serv_addr.sin_addr.s_addr = inet_addr("127.0.0.1");


    /* Create a socket first */
    if((sockfd1 = socket(AF_INET, SOCK_STREAM, 0))< 0)
    {
        printf("\n Error : Could not create socket \n");
        return 1;
    }

    /* Create a socket second */
    if((sockfd2 = socket(AF_INET, SOCK_STREAM, 0))< 0)
    {
        printf("\n Error : Could not create socket \n");
        return 1;
    }
    
    struct timeval tv;
    tv.tv_sec = 0;
    tv.tv_usec = 0;
    /* Open the file that we wish to transfer */
    FILE *fp = fopen("input.txt","rb");
    if(fp==NULL)
    {
        printf("File opern error");
        return 1;   
    }   
    if(connect(sockfd1, (struct sockaddr *)&serv_addr, sizeof(serv_addr))<0)
    {
        printf("\n Error : Connect Failed \n");
        return 1;
    }


    if(connect(sockfd2, (struct sockaddr *)&serv_addr, sizeof(serv_addr))<0)
    {
        printf("\n Error : Connect Failed \n");
        return 1;
    }
    
    int nread =0;
    int isLast =0;
    int state = 0;



    struct timeval timeout;
    timeout.tv_sec = TIME_OUT;
    timeout.tv_usec = 0;
    
    
    /* For timeout using SO_RCVTIMEO field , after TIME_OUT if no packet is there to be received
    it will resend the packets;
    */
    
    setsockopt(sockfd1, SOL_SOCKET, SO_RCVTIMEO, (const char*)&timeout, sizeof timeout);
    setsockopt(sockfd2, SOL_SOCKET, SO_RCVTIMEO, (const char*)&timeout, sizeof timeout);

    while(1){

            // signal(SIGALRM, handle_alarm);
            int maxfd = 0;

            FD_ZERO(&rset);
            if(sockfd1 > 0){
                FD_SET( sockfd1 , &rset);
                if(sockfd1 > maxfd)
                    maxfd = sockfd1;
            }
            if(sockfd2>0){
                FD_SET( sockfd2 , &rset);
                if(sockfd2 > maxfd)
                    maxfd = sockfd2;
            }

            int activity = select( maxfd + 1 , &rset , NULL , NULL , &tv); 

            if(FD_ISSET(sockfd1,&rset)){
                recv(sockfd1, &rcv_ack1, sizeof(rcv_ack1), 0);
                    if (rcv_ack1.hd.sq_no == offset1){
                        printf("RCVD ACK for packet with seq. no. %d through channel : %d\n", rcv_ack1.hd.sq_no, rcv_ack1.hd.channel_num);
                        // state = 0;
                        rec_flg1 = 1;
                }
            }

            if(FD_ISSET(sockfd2,&rset)){
                recv(sockfd2, &rcv_ack2, sizeof(rcv_ack2), 0);
                    if (rcv_ack2.hd.sq_no == offset2){
                        printf("RCVD ACK for packet with seq. no. %d through channel : %d\n", rcv_ack2.hd.sq_no, rcv_ack2.hd.channel_num);
                        rec_flg2 = 1;
                    }
            } 
            if(rec_flg1 == 0 && rec_flg2== 0){
                state = 1;
            }
            else if(rec_flg1 == 1 && rec_flg2==1){
                state = 0;
            }
            else if(rec_flg1 == 1 && rec_flg2==0){
                state = 2;
            }
            else{
                state = 3;
            }
            switch(state){

                case 0:{
                    
                    unsigned char buff[PACKET_SIZE+1]={'\0'};
                    offset1 = ftell(fp);

                    nread = fread(buff,1,PACKET_SIZE,fp);
                    if (nread < PACKET_SIZE)
                        isLast = 1;
                    send_pkt1 = create_packet(nread,offset1,isLast,0,1);
                    strcpy(send_pkt1.data,buff);


                    if(nread > 0)
                    {
                        printf("SEND PKT: Seq No: %d of Size: %d from channel: %d\n",send_pkt1.hd.sq_no,send_pkt1.hd.size,send_pkt1.hd.channel_num);
                        sendto(sockfd1, &send_pkt1, sizeof(send_pkt1), 0, (struct sockaddr * )&serv_addr, sizeof(serv_addr));
                        rec_flg1 = 0;
                        //alarm(0.01);
                    }

                    offset2 = ftell(fp);
                    
                    nread = fread(buff,1,PACKET_SIZE,fp);
                    if (nread < PACKET_SIZE && nread >0)
                        isLast = 1;
                    send_pkt2 = create_packet(nread,offset2,isLast,0,2);
                    strcpy(send_pkt2.data,buff);

                    if(nread > 0)
                    {
                        printf("SEND PKT: Seq No: %d of Size: %d from channel: %d\n",send_pkt2.hd.sq_no,send_pkt2.hd.size,send_pkt2.hd.channel_num);
                        sendto(sockfd2, &send_pkt2, sizeof(send_pkt2), 0, (struct sockaddr * )&serv_addr, sizeof(serv_addr));
                        rec_flg2 = 0;
                        //alarm(0.01);
                    }


                }break;
                case 1:{

                    if( byteRec =recv(sockfd1, &rcv_ack1, sizeof(rcv_ack1), 0) <0){
                        sendto(sockfd1, &send_pkt1, sizeof(send_pkt1), 0, (struct sockaddr * )&serv_addr, slen);
                        printf("Resending Packet for SEND PKT: Seq No: %d of Size: %d from channel: %d\n",send_pkt1.hd.sq_no,send_pkt1.hd.size,send_pkt1.hd.channel_num);
                    }
                    else{
                        if (rcv_ack1.hd.sq_no == offset1)
                        {
                           printf("RCVD ACK for packet with seq. no. %d through channel : %d\n", rcv_ack1.hd.sq_no, rcv_ack1.hd.channel_num);
                           rec_flg1 =1;
                        }
                        
                    }


                    if( byteRec =recv(sockfd2, &rcv_ack2, sizeof(rcv_ack2), 0) <0){
                        sendto(sockfd2, &send_pkt2, sizeof(send_pkt2), 0, (struct sockaddr * )&serv_addr, slen);
                        printf("SEND PKT: Seq No: %d of Size: %d from channel: %d\n",send_pkt2.hd.sq_no,send_pkt2.hd.size,send_pkt2.hd.channel_num);
                    }
                    else{
                        if(rcv_ack2.hd.sq_no == offset2){
                            printf("RCVD ACK for packet with seq. no. %d through channel : %d\n", rcv_ack2.hd.sq_no, rcv_ack2.hd.channel_num);
                            rec_flg2 =1;
                        }
                    }


                }break;
                case 2:{

                    offset1 = ftell(fp);

                    int nread = fread(buff,1,PACKET_SIZE,fp);
                    if (nread < PACKET_SIZE)
                        isLast = 1;
                    send_pkt1 = create_packet(nread,offset1,isLast,0,1);
                    strcpy(send_pkt1.data,buff);


                    if(nread > 0)
                    {
                        printf("SEND PKT: Seq No: %d of Size: %d from channel: %d\n",send_pkt1.hd.sq_no,send_pkt1.hd.size,send_pkt1.hd.channel_num);
                        sendto(sockfd1, &send_pkt1, sizeof(send_pkt1), 0, (struct sockaddr * )&serv_addr, sizeof(serv_addr));
                        rec_flg1 = 0;
                        //alarm(0.01);
                    }


                    if( byteRec =recv(sockfd2, &rcv_ack2, sizeof(rcv_ack2), 0) <0){
                        sendto(sockfd2, &send_pkt2, sizeof(send_pkt2), 0, (struct sockaddr * )&serv_addr, slen);
                        printf("RCVD ACK for packet with seq. no. %d through channel : %d\n", rcv_ack2.hd.sq_no, rcv_ack2.hd.channel_num);
                    }
                    else{
                        if(rcv_ack2.hd.sq_no == offset2){
                            printf("RCVD ACK for packet with seq. no. %d through channel : %d\n", rcv_ack2.hd.sq_no, rcv_ack2.hd.channel_num);
                            rec_flg2 =1;
                        }
                    }

                }break;

                case 3:{


                    offset2 = ftell(fp);
                    
                    nread = fread(buff,1,PACKET_SIZE,fp);
                    if (nread < PACKET_SIZE && nread >0)
                        isLast = 1;
                    send_pkt2 = create_packet(nread,offset2,isLast,0,2);
                    strcpy(send_pkt2.data,buff);

                    if(nread > 0)
                    {
                        printf("SEND PKT: Seq No: %d of Size: %d from channel: %d\n",send_pkt2.hd.sq_no,send_pkt2.hd.size,send_pkt2.hd.channel_num);
                        sendto(sockfd2, &send_pkt2, sizeof(send_pkt2), 0, (struct sockaddr * )&serv_addr, sizeof(serv_addr));
                        rec_flg2 = 0;
                        //alarm(0.01);
                    }
                    if( byteRec =recv(sockfd1, &rcv_ack1, sizeof(rcv_ack1), 0) <0){
                        sendto(sockfd1, &send_pkt1, sizeof(send_pkt1), 0, (struct sockaddr * )&serv_addr, slen);
                        printf("Resending Packet for SEND PKT: Seq No: %d of Size: %d from channel: %d\n",send_pkt1.hd.sq_no,send_pkt1.hd.size,send_pkt1.hd.channel_num);
                    }
                    else{
                        if (rcv_ack1.hd.sq_no == offset1)
                        {
                           printf("RCVD ACK for packet with seq. no. %d through channel : %d\n", rcv_ack1.hd.sq_no, rcv_ack1.hd.channel_num);
                           rec_flg1 =1;
                        }
                        
                    }

                }break;

            }
            if((rec_flg1 ==1 && rec_flg2==1) && isLast == 1)
                break;    

    }

    fclose(fp);
    return 0;
}