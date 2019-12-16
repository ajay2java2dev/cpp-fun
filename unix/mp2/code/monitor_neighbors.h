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

void logToFile(char *theLogFileName, char *logLine) {
	if (theLogFileName != NULL) {
		FILE *theLogFile = fopen(theLogFileName, "a");
		fprintf(theLogFile, "%s",logLine);
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
							
							// printf("\n #--------------------------#");
							// printf("\ni : %d, j : %d, k : %d", i, j, k);
							// printf("\ni_node : %d, j_node : %d, k_node : %d", i_node, j_node, k_node);
							// printf("\nft[i_node].dist[j_node]: %d", ft[i_node].dist[j_node]);
							// printf("\ncost_matrix[i_node][k_node]: %d", cost_matrix[i_node][k_node]);
							// printf("\nft[k_node].dist[j_node]: %d", ft[k_node].dist[j_node]);
							// printf("\n #--------------------------#");

							ft[i_node].dist[j_node] = ft[i_node].dist[k_node] + ft[k_node].dist[j_node];
							ft[i_node].dist[j_node] = k_node;
							count++;
						}
					}
				}
			}
		} while (count != 0);

	}
}

void* invokeDVCalculation(void* unusedParam) {
	calculateEfficientDistanceVector (node_size, cost_matrix);
}



/*
int minimumDistance (int dist[], bool sptSet[]) {
	int min = INT8_MAX, min_index;

	for (int node = 0; node < M; node++) {
		if (sptSet[node] == false && dist[node] <= min) {
			min = dist[node], min_index = node;			
		}
	}
	//printf("\nreturning minimum index : %d and min value %d", min_index, min);
	return min_index;
}

//single source shorted path algorithm
void calculateShortestPathForAllNodes(int graph[M][N], int src, int *dist) {
	
	//removed dist from local param to pass a param due to memory constraints.
	//int dist[M]; //to hold the shortest distance from src to node/vertex i.
	
	bool sptSet[M]; //node/vertex i will be true if node is included in shortest path

	for (int i = 0; i < M; i++) {
		dist[i] = INT32_MAX, sptSet[i] = false; //resetting out temp array
	}

	dist[src] = 0; //distance of the global node to itself is 0;

	//relaxation for all nodes
	for (int count = 0; count < M-1; count++) {
		int u = minimumDistance(dist, sptSet);
		
		sptSet[u] = true;

		for (int v = 0; v < M; v++) {
			if (!sptSet[v] && graph[u][v] && dist[u] != INT32_MAX
				&& dist[u] + graph[u][v] < dist[v]) {
				dist[v] = dist[u] + graph[u][v];
			}
		}
	}


	printf("Vertex \t\t Distance from Source \n");
	for (int i =0; i < M; i++)
		printf("%d \t\t %d\n",i,dist[i]);	
}
*/

void sendOrForwardToDestination (int dest_node, char *message) {
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
	
	//FIXME: Do we need to set the broadcast settings ... 
	their_addr.sin_family = AF_INET;
	their_addr.sin_port = htons(7777);
	inet_pton(AF_INET, theirAddr, &their_addr.sin_addr);

	if ((numbytes = sendto(sockfd,message, strlen(message), 0,
		(struct sockaddr *)&their_addr, sizeof their_addr)) == -1) {
		perror("sendto");
		exit(1);
	}
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

	printf("\n\nYeahhh..... Ready for listening .....");
	printf("\n\nYeahhh..... Ready for listening .....");

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
				
		//unsigned short int le_value = (recvBuf+4)[0] + ((recvBuf+4)[1] << 8); //little_endian value
		unsigned short int dest_node = (recvBuf+4)[1] + ((recvBuf+4)[0] << 8); //big_endian value
		printf("\n\nRequesting packet to destination : %d",dest_node);
		
		char * message = recvBuf+6;
		
		int cost = ft[globalMyID].dist[dest_node];
		int nextHop = ft[globalMyID].from[dest_node];

		printf("\n%s heard from %s, dest_node : %d, msg recvd: %s",recvBuf,fromAddr,dest_node,message);
		printf("\ncost from %d to %d seems to be : %d",globalMyID,nextHop,cost);
		printf("\n......");
		
		short int heardFrom = -1;
		if(strstr(fromAddr, "10.1.1."))
		{
			heardFrom = atoi(strchr(strchr(strchr(fromAddr,'.')+1,'.')+1,'.')+1);
			
			//TODO: this node can consider heardFrom to be directly connected to it; 
			//do any such logic now.	
			//record that we heard from heardFrom just now.
			sprintf(logLine,"heard from %d just now\n",heardFrom);
			logToFile(log_file_name,logLine);
			if (globalMyID == dest_node) {
				sprintf(logLine, "receive packet message %s\n", message);
				logToFile(log_file_name,logLine);
			}
			gettimeofday(&globalLastHeartbeat[heardFrom], 0);
		}
		
		//Is it a packet from the manager? (see mp2 specification for more details)
		//send format: 'send'<4 ASCII bytes>, destID<net order 2 byte signed>, <some ASCII message>
		if(!strncmp(recvBuf, "send", 4))
		{
			//TODO send the requested message to the requested destination node
			// ... 
			if (cost >= INT16_MAX) {
				
				sprintf(logLine, "unreachable dest %d\n",dest_node);
				logToFile(log_file_name,logLine);

			} else if (nextHop == dest_node) {
				sprintf(logLine, "sending packet dest %d nexthop %d message %s\n",dest_node,nextHop,message);
				logToFile(log_file_name,logLine);
				sendOrForwardToDestination(dest_node, message);
			} else {
				sprintf(logLine, "forward packet dest %d nexthop %d message %s\n",dest_node,nextHop,message);
				logToFile(log_file_name,logLine);
				sendOrForwardToDestination(dest_node, message);
			}
		}
		//'cost'<4 ASCII bytes>, destID<net order 2 byte signed> newCost<net order 4 byte signed>
		else if(!strncmp(recvBuf, "cost", 4))
		{
			//TODO record the cost change (remember, the link might currently be down! 
			//in that case,
			//this is the new cost you should treat it as having once it comes back up.)
			// ...
			
			int new_cost = 0;
			sscanf(message, "%d", &new_cost);
			cost_matrix[globalMyID][dest_node] = new_cost;

			if (nextHop == dest_node) {
				calculateEfficientDistanceVector(node_size, cost_matrix);
			} else  {				
				sendOrForwardToDestination(dest_node, message);
			}

			sprintf(logLine, "\nnew cost to dest %d is %s\n",dest_node,message);
			printf("%s",logLine);			
		}
			
		//do other changes here");
		
		//TODO now check for the various types of packets you use in your own protocol
		//else if(!strncmp(recvBuf, "your other message types", ))
		// ...
	}
	//(should never reach here)
	close(globalSocketUDP);
}

