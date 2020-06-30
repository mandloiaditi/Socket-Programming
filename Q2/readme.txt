// compiled and checked on Ubuntu 18.04,  gcc version : gcc (Ubuntu 7.5.0-3ubuntu1~18.04) 7.5.0

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
	
	diff input.txt output.txt

d) Repeat steps (a) to (c) for different PACKET_SIZE and PDR in packet.h


NOTE: the arg1 '0' and '1' in ./relay arg1 are respectively to set ports for relay nodes 
corresponding to even numbered packets and odd numbered packets.





C. Methodology used:


1) For multiple timers using ISR have been implemented for each packet sent and timers are running per 
packet basis using signal handlers. The track of packet to be send is kept in siginfo_t * si pointer's
internal pointers which is passed into signal handler for the timer_id and the packet corresponding to which 
timeout has occured.
As soon as acknowledgement for a particular in window packet is received , timer_id corresponding to it
is deleted.

2) window sliding : Implemeted as per SR protocol i.e. as soon as lowest sq no packet's ack is received 
window is slided till next lowest unacked seq no.Similarly for receiver_window, it is slided as soon as lowest
sq_no packet is received by the amount that takes it to seq_no of next not-yet-received pkt.

3)Sequence Numbers: 
For ease of coding number of sequence number is taken to be much much larger than theoretical lower bound
of (2 * WINDOW_SIZE). Sequence numbers incremented by one and not in circular fashion.

4)For handling two relays two different port numbers have been used and there is one socket for each of the 
nodes being set up using UDP PROTOCOL.

5)sendto and receivefrom have been made non blocking where required using sockpocket.

6)Buffering implemented using fixed size. If the buffer is full packets may drop at server side too. But this is not very
likely for low PDR as buffer is continuosly written as and when in order packets arrive.

7)Delays in packets implemented using usleep() call.
