// compiled and checked on Ubuntu 18.04,  gcc version : gcc (Ubuntu 7.5.0-3ubuntu1~18.04) 7.5.0


A. requirements : A file named input.txt in current directory Q1
B. Execution Instructions:

a)compiling 

	gcc -o client client.c  

	// NOTE In case of linking error -lrt flag required for linking certain capabilities of <time.h> used in the code.

	gcc -o server server.c


b) Please run the files in following order on the terminals
	
	./server
	./client


c) May verify the output file using
 
	diff input.txt output.txt


d) Repeat steps (a) to (c) for different PACKET_SIZE and PDR in packet.h



C. Methodologies used:

1) For transmitting over multiple channels two sockets were created one for each connection from the client to server.

2) Timeout implemented using SO_RCVTIMEO field by using setsockopt on recv, after TIME_OUT if no packet is there,the packet is resent.

3) Buffering : All fwrites takes place continuously as and when in order packets arrive. Out of order packets are buffered 
and they are also written in file as soon as missing packets arrive. Thi strategy requires very less buffering.