
## Problem 1 : Stop And Wait (TCP)

### How to run the program


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
	using editor like gedit 
		OR
	diff input.txt ouput.txt


### Methodology
1) For transmitting over multiple channels two sockets were created one for each connection from the client to server.

2) Timeout implemented using SO_RCVTIMEO field by using setsockopt on recv, after TIME_OUT if no packet is there,the packet is resent.

3) Buffering : All fwrites takes place continuously as and when in order packets arrive. Out of order packets are buffered 
and they are also written in file as soon as missing packets arrive. Thi strategy requires very less buffering.


------------------------------------------------------------------------------------------------------------------------------------------------
## Problem 2 : Selective Repeat(UDP)

### How to run the program


A. Requirements : A file named "input.txt" in current directory Q2.
B. Execution Instructions:
a) Compiling

	gcc -o client client.c -lrt 
	// NOTE -lrt flag required for linking certain capabilities of <time.h> used in the code.
	gcc -o relay relay.c
	gcc -o server server.c
b) Please run the following on 4 different terminals in following order:
	./server
	./relay  0 
	./relay  1
	./client
c)May verify the the output using
	using editor like gedit 
		OR
	diff input.txt ouput.txt


### Notes for running the code
NOTE: -lrt flag required for linking certain capabilities of <time.h> used in the code.

NOTE: the arg1 '0' and '1' in ./relay arg1 are respectively to set ports for relay nodes 
corresponding to even numbered packets and odd numbered packets.


### Methodology

1) For multiple timers using ISR have been implemented for each packet sent and timers are running per 
packet basis using signal handlers. The track of packet to be send is kept in siginfo_t * si pointer's
internal pointers which is passed into signal handler for the timer_id and the packet corresponding to which 
timeout has occured.
As soon as acknowledgement for a particular in window packet is received , timer_id corresponding to it
is deleted.

2) window sliding : Implemeted as per SR protocol i.e. as soon as lowest sq no packet's ack is received 
window is slided till next lowest unacked seq no.Similarly for receiver_window, it is slided as soon as lowest
sq_no packet is received by the amount that takes it to seq_no of next not-yet-received pkt.

3) Sequence Numbers: 
For ease of coding number of sequence number is taken to be much much larger than theoretical lower bound
of (2 * WINDOW_SIZE). Sequence numbers incremented by one and not in circular fashion.

4) For handling two relays two different port numbers have been used and there is one socket for each of the 
nodes being set up using UDP PROTOCOL.

5) sendto and receivefrom have been made non blocking where required using sockpocket.

6) Buffering implemented using fixed size. If the buffer is full packets may drop at server side too. But this is not very
likely for low PDR as buffer is continuosly written as and when in order packets arrive.

7) Delays in packets implemented using usleep() call.

