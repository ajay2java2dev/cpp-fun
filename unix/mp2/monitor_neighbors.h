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

#include<stdbool.h>

#define M 256
#define N 256

//forwarding / routing table to be constructed for each node
struct fw_table {
	int dist[M];
	int from[N];
}ft[M];

extern int cost_matrix[M][N];

extern char *log_file_name;

extern int node_size;
extern int *node_list;

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
			int num_bytes_sent = sendto(globalSocketUDP, buf, length, 0,
				  (struct sockaddr*)&globalNodeAddrs[i], sizeof(globalNodeAddrs[i]));
			//printf("\ninside hacky %i: global id: %d, buf : %d",i,globalMyID, num_bytes_sent);
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

void logToFile(char *theLogFileName, char *logLine) {
	if (theLogFileName != NULL) {
		FILE *theLogFile = fopen(theLogFileName, "a");
		fprintf(theLogFile, "%s",logLine);
		
		printf("%s",logLine);
		fclose(theLogFile);
	} else {
		printf("Unable to print to file. The log file name is null ...");
	}
}

void printGraph(int graph[M][N]) {
	if (graph != NULL) {
		printf("\n################################\n");
		for (int i = 0; i < M; i++) {
			for (int j = 0; j < N; j++) {
				if (graph[i][j] > 0) {
					printf("\n| [%d][%d], cost  = %d |",i,j,graph[i][j]);
				}
			}			
		}
		printf("\n################################\n");
	}
}

void printCostMatrix() {
	for (int i = 0; i < M; i++) {
		for (int j = 0; j < M; j++) {
			if (cost_matrix[i][j] > 0 
				&& cost_matrix[i][j] < INT16_MAX) {
					printf("\n cost %d to %d is .. %d"
						,i,j,cost_matrix[i][j]);
				}
		}
	}
}

void constructNetworkTopology(int cost_matrix[M][N]) {
	for (int i = 0; i < M; i++) {
		for (int j =0; j < N; j++) {
			ft[i].dist[j] = cost_matrix[i][j];
			ft[i].from[j] = j;
		}
	}
}

void calculateEfficientDistanceVector (int num_nodes, int cost_matrix[M][N]) {

	if (cost_matrix != NULL) {
		int count;

		do {
			count = 0;
			for (int i =0; i < num_nodes; i++) {
				int i_node = node_list[i];
				for (int j = 0; j < num_nodes; j++) {
					int j_node = node_list[j];
					for (int k = 0; k < num_nodes; k++) {
						int k_node = node_list[k];
						
						if (ft[i_node].dist[j_node] > cost_matrix[i_node][k_node] + ft[k_node].dist[j_node]) {
							
							printf("\n #--------------------------#");
							printf("\ni : %d, j : %d, k : %d", i, j, k);
							printf("\ni_node : %d, j_node : %d, k_node : %d", i_node, j_node, k_node);
							printf("\nft[i_node].dist[j_node]: %d", ft[i_node].dist[j_node]);
							printf("\ncost_matrix[i_node][k_node]: %d", cost_matrix[i_node][k_node]);
							printf("\nft[k_node].dist[j_node]: %d", ft[k_node].dist[j_node]);
							printf("\n #--------------------------#");
							
							ft[i_node].dist[j_node] = ft[i_node].dist[k_node] + ft[k_node].dist[j_node];
							ft[i_node].dist[j_node] = k_node;
							count++;
						}
					}
				}
			}
		} while (count != 0);

		printf("\nnumber of nodes processed : %d",num_nodes);
	}
}

void* invokeDVCalculation(void* unusedParam) {
	calculateEfficientDistanceVector (node_size, cost_matrix);
}

int sndToDest(int src_node, int dest_node, char *command, char *message) {
	short int destID = dest_node;
	short int no_destID = htons(destID);

	printf("\n\nDESTINATION ID : %d",destID);
	printf("\nNO_DEST ID : %d",no_destID);
	printf("\nCOMMAND :%s\n\n",command);

	int senderSocket = socket(AF_INET, SOCK_DGRAM, 0);
	if(senderSocket < 0)
		perror("socket()");
	
	struct sockaddr_in srcAddr;
	char tempaddr1[100];
	sprintf(tempaddr1, "10.1.1.%d", src_node);
	memset(&srcAddr, 0, sizeof(srcAddr));
	srcAddr.sin_family = AF_INET;
	srcAddr.sin_port = htons(8999);
	inet_pton(AF_INET, tempaddr1, &srcAddr.sin_addr);
	if(bind(senderSocket, (struct sockaddr*)&srcAddr, sizeof(srcAddr)) < 0)
		perror("bind()");
	
	struct sockaddr_in destAddr;
	char tempaddr[100];
	sprintf(tempaddr, "10.1.1.%d", dest_node);
	memset(&destAddr, 0, sizeof(destAddr));
	destAddr.sin_family = AF_INET;
	destAddr.sin_port = htons(7777);
	inet_pton(AF_INET, tempaddr, &destAddr.sin_addr);
	
	int msgLen = 4+sizeof(short int)+strlen(message);
	char* sendBuf = malloc(msgLen);

	strcpy(sendBuf, command);
	memcpy(sendBuf+4, &no_destID, sizeof(short int));
	memcpy(sendBuf+4+sizeof(short int), message, strlen(message));

	if(sendto(senderSocket, sendBuf, msgLen, 0, (struct sockaddr*)&destAddr, sizeof(destAddr)) < 0)
		perror("sendto()");
	free(sendBuf);

	
	close(senderSocket);

}

int sendOrForwardToDestination (int dest_node, char *message) {
	int numbytes;
	int sockfd;
	int broadcast = 1;
	struct sockaddr_in their_addr;
	char theirAddr[100];

	if ((sockfd = socket(AF_INET, SOCK_DGRAM, 0)) == -1) {
		perror("socket");
		exit(1);
	}

	if (setsockopt(sockfd, SOL_SOCKET, SO_BROADCAST, &broadcast,
		sizeof broadcast) == -1) {
		perror("setsockopt (SO_BROADCAST)");
		exit(1);
	}
	
	sprintf(theirAddr, "10.1.1.%d", dest_node);	
	printf("\n%s\n", theirAddr);

	memset(&theirAddr, 0, sizeof(theirAddr));
	
	their_addr.sin_family = AF_INET;
	their_addr.sin_port = htons(7777);
	inet_pton(AF_INET, theirAddr, &their_addr.sin_addr);

	if ((numbytes = sendto(sockfd,message, strlen(message), 0,
		(struct sockaddr *)&their_addr, sizeof their_addr)) == -1) {
		perror("sendto");
		exit(1);
	}

	return numbytes;
}

int findNextHop(int src_node, int dest_node) {
	int next_hop = -1;
	int prev_idx = -1;
	int prev_cost = INT32_MAX;

	for (int i =0; i< sizeof(ft[src_node]); i++) {
		
		int tmp_cost = ft[globalMyID].dist[i];
		int tmp_next_hop = ft[globalMyID].from[i];

		if (globalMyID != i && tmp_cost >= 0 
				&& tmp_cost <= 1000 && tmp_next_hop <= 255) {
			printf("\nnext hops available from %d are %d with cost %d", 
				globalMyID, tmp_next_hop, tmp_cost);
			
			if (next_hop != -1) {
				//this means next hop already set... do tie breaker
				if (prev_cost > tmp_cost) {
					next_hop = tmp_next_hop;
					prev_idx = i;
					prev_cost = tmp_cost;
				} else if (prev_cost == tmp_cost) {
					if (tmp_next_hop < next_hop) {
						next_hop = tmp_next_hop;
						prev_idx = i;
						prev_cost = tmp_cost;
					}
				}
			} else {
				next_hop = tmp_next_hop;
				prev_idx = i;
				prev_cost = tmp_cost;
			}
		}
	}

	return next_hop;
}

void listenForNeighbors()
{
	char fromAddr[100];
	struct sockaddr_in theirAddr;
	socklen_t theirAddrLen;
	unsigned char recvBuf[1000];
	
	char logLine[1024] = {};
	FILE *theLogFile = fopen(log_file_name, "w+");

	int bytesRecvd;
	printf("\nlistener waiting for packet to arrive...");

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

		recvBuf[bytesRecvd] = '\0';

		printf("\nheard from %s  and strstr(fromAddr, \"10.1.1.\") is = %s"
			,fromAddr, strstr(fromAddr, "10.1.1."));
		
		short int heardFrom = -1;
		if(strstr(fromAddr, "10.1.1."))
		{
			heardFrom = atoi(strchr(strchr(strchr(fromAddr,'.')+1,'.')+1,'.')+1);
			
			//TODO: this node can consider heardFrom to be directly connected to it; do any such logic now.	
			if (globalMyID != heardFrom 
					&& cost_matrix[globalMyID][heardFrom] == INT16_MAX) {
				cost_matrix[globalMyID][heardFrom] = 1;
				cost_matrix[heardFrom][globalMyID] = 1;
				node_list[node_size] = heardFrom;
				node_size = node_size + 1;
				
				constructNetworkTopology(cost_matrix);
				calculateEfficientDistanceVector(node_size, cost_matrix);
			}
			//record that we heard from heardFrom just now.
			
			sprintf(logLine,"\nheard from %d just now\n",heardFrom);
			printf("%s",logLine);

			gettimeofday(&globalLastHeartbeat[heardFrom], 0);
		}
		
		//Is it a packet from the manager? (see mp2 specification for more details)
		//send format: 'send'<4 ASCII bytes>, destID<net order 2 byte signed>, <some ASCII message>
		if(!strncmp(recvBuf, "send", 4))
		{
			//TODO send the requested message to the requested destination node
			// ... 
			unsigned int dest_node = (recvBuf+4)[1] + ((recvBuf+4)[0] << 8); //big_endian value
			char * message = recvBuf+4+sizeof(short int);

			printf("\nCOMMAND \"%s\" heard from %s, dest_node : %d, msg recvd: %s",recvBuf,fromAddr,dest_node,message);

			int nextHop = ft[globalMyID].dist[dest_node];
			printf("\nPFWD MSG ... next hop %d",nextHop);

			if (nextHop < 0 || nextHop > 255) {
				nextHop  = findNextHop(globalMyID, dest_node);
				short int no_destID = htons(nextHop);
			}			
			
			int cost = ft[globalMyID].dist[nextHop];			
						
			printf("\nFINAL cost from %d to next hop %d  is : %d",globalMyID,nextHop,cost);
			printf("\n......\n");

			int sent_bytes = -1;
			if (cost > 0 && cost < 1000) {
				sprintf(logLine, "sending packet dest %d nexthop %d message %s\n",dest_node,nextHop,message);
				logToFile(log_file_name,logLine);
				
				sent_bytes = sendOrForwardToDestination(nextHop, message);
				//sndToDest(globalMyID, dest_node, "pfwd", message);
				printf("\nsent %s to next_hop %d", message, nextHop);
			}
		}
		//'cost'<4 ASCII bytes>, destID<net order 2 byte signed> newCost<net order 4 byte signed>
		else if(!strncmp(recvBuf, "cost", 4))
		{
			//TODO record the cost change (remember, the link might currently be down! 
			//in that case,
			//this is the new cost you should treat it as having once it comes back up.)
			// ...
			//unsigned short int le_value = (recvBuf+4+sizeof(int))[0] + ((recvBuf+4+sizeof(short int))[1] << 8);
			unsigned int dest_node = (recvBuf+4)[1] + ((recvBuf+4)[0] << 8); //big_endian value
			unsigned int new_cost = (recvBuf+4+sizeof(int))[1] + ((recvBuf+4+sizeof(short int))[0] << 8);

			sprintf(logLine, "\nnew cost from %d to %d is %d",globalMyID, dest_node, new_cost);
			
			cost_matrix[globalMyID][dest_node] = new_cost;
			cost_matrix[dest_node][globalMyID] = new_cost;
			constructNetworkTopology(cost_matrix);
			calculateEfficientDistanceVector(node_size, cost_matrix);
			printCostMatrix();

		} else if(!strncmp(recvBuf, "pfwd", 4)) {
			
			unsigned int dest_node = (recvBuf+4)[1] + ((recvBuf+4)[0] << 8); //big_endian value
			
			char * message = recvBuf+4+sizeof(short int);

			printf("\nPFWD MSG ...COMMAND \"%s\" heard from %s, dest_node : %d, msg recvd: %s"
				,recvBuf,fromAddr,dest_node,message);

			int nextHop = ft[globalMyID].dist[dest_node];
			printf("\nPFWD MSG ... next hop %d",nextHop);

			if (nextHop < 0 || nextHop > 255) {
				nextHop = ft[globalMyID].dist[nextHop];
				printf("\nPFWD MSG ... NEW next hop %d",nextHop);
			}					
			int cost = ft[globalMyID].dist[nextHop];
			
			printf("\nPFWD MSG ...FINAL cost from %d to next hop %d  is : %d"
					,globalMyID,nextHop,cost);
			printf("\n......\n");

			int sent_bytes = -1;

			if (cost > 1000) {
				
				sprintf(logLine, "unreachable dest %d\n",nextHop);
				logToFile(log_file_name,logLine);
			
			} else {
				if (cost > 0 && cost < 1000) {

					sprintf(logLine, "forward packet dest %d nexthop %d message %s\n",dest_node,nextHop,message);
					logToFile(log_file_name,logLine);
					
					if (dest_node == nextHop) {
						sent_bytes = sndToDest(globalMyID, dest_node, "rcvd", message);
					} else {
						sent_bytes = sndToDest(globalMyID, dest_node, "pfwd", message);
					}
				}
			}
			
		
		} else if(!strncmp(recvBuf, "rcvd", 4)) {

			unsigned int dest_node = (recvBuf+4)[1] + ((recvBuf+4)[0] << 8); //big_endian value
			char * message = recvBuf+4+sizeof(short int);

			printf("\nRCVD MSG ... %s heard from %s, dest_node : %d, msg recvd: %s",recvBuf,fromAddr,dest_node,message);
			sprintf(logLine, "receive packet message %s\n",message);
			logToFile(log_file_name,logLine);
		}
			
		//do other changes here");
		
		//TODO now check for the various types of packets you use in your own protocol
		//else if(!strncmp(recvBuf, "your other message types", ))
		// ...
	}
	//(should never reach here)
	close(globalSocketUDP);
}

