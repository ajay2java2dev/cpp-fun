#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <errno.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <sys/time.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <netdb.h>
#include <pthread.h>

#define M 255
#define N 255

extern int fw_table[M][N];

extern int globalMyID;
//last time you heard from each node. TODO: you will want to monitor this
//in order to realize when a neighbor has gotten cut off from you.
extern struct timeval globalLastHeartbeat[256];

//our all-purpose UDP socket, to be bound to 10.1.1.globalMyID, port 7777
extern int globalSocketUDP;
//pre-filled for sending to 10.1.1.0 - 255, port 7777
extern struct sockaddr_in globalNodeAddrs[256];

//Yes, this is terrible. It's also terrible that, in Linux, a socket
//can't receive broadcast packets unless it's bound to INADDR_ANY,
//which we can't do in this assignment.
void hackyBroadcast(const char* buf, int length)
{
	int i;
	for(i=0;i<256;i++){
		 //(although with a real broadcast you would also get the packet yourself)
		if(i != globalMyID) {
			//printf("\ninside hacky %i: global id: %d",i,globalMyID);
			sendto(globalSocketUDP, buf, length, 0,
				  (struct sockaddr*)&globalNodeAddrs[i], sizeof(globalNodeAddrs[i]));
		}
	}
}

void* announceToNeighbors(void* unusedParam)
{
	struct timespec sleepFor;
	sleepFor.tv_sec = 0;
	sleepFor.tv_nsec = 300 * 1000 * 1000; //300 ms
	while(1)
	{
		hackyBroadcast("HEREIAM", 7);
		nanosleep(&sleepFor, 0);
	}
}

void listenForNeighbors()
{
	//printf("\ninside listen for neighbour");
	FILE *theLogFile = fopen ("log.txt", "w");
		
	char fromAddr[100];
	struct sockaddr_in theirAddr;
	socklen_t theirAddrLen;
	unsigned char recvBuf[1000];

	int bytesRecvd;
	while(1)
	{
		theirAddrLen = sizeof(theirAddr);
		if ((bytesRecvd = recvfrom(globalSocketUDP, recvBuf, 1000 , 0, 
					(struct sockaddr*)&theirAddr, &theirAddrLen)) == -1)
		{
			perror("connectivity listener: recvfrom failed");
			exit(1);
		}
		
		inet_ntop(AF_INET, &theirAddr.sin_addr, fromAddr, 100);

		char logLine[1024];
		recvBuf[bytesRecvd] = '\0';
		
		/*
		unsigned short int le_value = (recvBuf+4)[0] + ((recvBuf+4)[1] << 8);
		*/
		unsigned short int be_value = (recvBuf+4)[1] + ((recvBuf+4)[0] << 8);
		int nextHop = -1;
		
		//printf("\naction %s heard from %s, dest_node : %d, msg recvd: %s",recvBuf,fromAddr,be_value,recvBuf+6);

		if (fw_table == NULL) {
			perror("Forwarding table is empty: cannot proceed");
			exit(1);
		}
		
		int arr[M];
		int nearest_neig_cost = -1;

		int cost = fw_table[globalMyID][be_value];
		nextHop = be_value;
		printf("cost %d\n",cost);
		
		if (cost <= 0) {
			memcpy(arr,fw_table[globalMyID],N);

			for (int i = 0; i < sizeof(arr); i++) {
				if (arr[i] > 0){
					printf("\nnode : %d\n",arr[i]); 	
					if (i == be_value) {
						nextHop = i;
						break;
					}
				}
			}
		}
		
		short int heardFrom = -1;
		if(strstr(fromAddr, "10.1.1."))
		{
			heardFrom = atoi(strchr(strchr(strchr(fromAddr,'.')+1,'.')+1,'.')+1);
			
			//TODO: this node can consider heardFrom to be directly connected to it; do any such logic now.	
			//record that we heard from heardFrom just now.

			sprintf(logLine, "heard heartbeat from ... %d",heardFrom);
			printf("%s",logLine);

			gettimeofday(&globalLastHeartbeat[heardFrom], 0);
		}
		
		//Is it a packet from the manager? (see mp2 specification for more details)
		//send format: 'send'<4 ASCII bytes>, destID<net order 2 byte signed>, <some ASCII message>
		if(!strncmp(recvBuf, "send", 4))
		{
			//TODO send the requested message to the requested destination node
			// ...
			sprintf(logLine, "sending packet dest %d nexthop %d message %s\n",be_value,nextHop,recvBuf+6);
			printf("%s",logLine);

			fwrite(logLine, 1, strlen(logLine),theLogFile);
		}
		//'cost'<4 ASCII bytes>, destID<net order 2 byte signed> newCost<net order 4 byte signed>
		else if(!strncmp(recvBuf, "cost", 4))
		{
			//TODO record the cost change (remember, the link might currently be down! in that case,
			//this is the new cost you should treat it as having once it comes back up.)
			// ...
			sprintf(logLine, "sending packet dest %d nexthop %d message %s\n",be_value,nextHop,recvBuf+6);
			printf("%s",logLine);

			fwrite(logLine, 1, strlen(logLine),theLogFile);
		}
		
		
		//printf("\ndo other changes here");
		//TODO now check for the various types of packets you use in your own protocol
		//else if(!strncmp(recvBuf, "your other message types", ))
		// ... 
	}
	//(should never reach here)
	close(globalSocketUDP);
}

