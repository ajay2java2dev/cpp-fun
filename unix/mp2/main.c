#include <stdio.h>
#include <stdlib.h>
#include <pthread.h>
#include <limits.h>
#include "monitor_neighbors.h"

void listenForNeighbors();
void* announceToNeighbors(void* unusedParam);

char *log_file_name;

int cost_matrix[M][N];

int node_size = 0;
int* node_list;
int globalMyID = 0;

//last time you heard from each node. TODO: you will want to monitor this
//in order to realize when a neighbor has gotten cut off from you.
struct timeval globalLastHeartbeat[256];

//our all-purpose UDP socket, to be bound to 10.1.1.globalMyID, port 7777
int globalSocketUDP;
//pre-filled for sending to 10.1.1.0 - 255, port 7777
struct sockaddr_in globalNodeAddrs[256];

//struct fw_table ft[M] = {0};

int main(int argc, char** argv)
{
	if(argc != 4)
	{
		fprintf(stderr, "Usage: %s mynodeid initialcostsfile logfile\n\n", argv[0]);
		exit(1);
	}

	//initialization: get this process's node ID, record what time it is, 
	//and set up our sockaddr_in's for sending to the other nodes.
	globalMyID = atoi(argv[1]);
	int i;

	for(i=0;i<256;i++)
	{
		gettimeofday(&globalLastHeartbeat[i], 0);
		char tempaddr[100];
		sprintf(tempaddr, "10.1.1.%d", i);
		memset(&globalNodeAddrs[i], 0, sizeof(globalNodeAddrs[i]));		
		globalNodeAddrs[i].sin_family = AF_INET;
		globalNodeAddrs[i].sin_port = htons(7777);
		inet_pton(AF_INET, tempaddr, &globalNodeAddrs[i].sin_addr);		
	}

	//TODO: read and parse initial costs file. default to cost 1 if no entry for a node. file may be empty.
	log_file_name = argv[3];
	
	FILE *file = fopen (argv[2], "r");
	if (file == NULL) {
		perror("Unable to open file!");
		exit(1);
	}
	
	node_list = (int*) malloc (M * sizeof(int*));
	char line[128];

	for (int i =0; i < M ; i++) {	
		node_list[i] = -1;
		for (int j =0; j < N ; j++) {
			cost_matrix[i][i] = 0;
			cost_matrix[i][j] = INT16_MAX;
		}
	}

	node_list[node_size] = globalMyID;
	node_size = node_size + 1;

	while (fgets(line, sizeof line, file) != NULL) {
		
		struct addrinfo *res;
		int sockfd;
		char tempaddr[100];		
		
		char *token = strtok(line, " ");
		int count = 0;
		int cost = 1;
		int nodeid = 0;
		while (token != NULL) {

			if (count ==0) {
				nodeid = atoi(token);
				sprintf(tempaddr, "10.1.1.%d",nodeid);
				count++;	
			} else {
				cost = atoi(token);
			}

			token = strtok(NULL, "-");
		}
		
		if (globalMyID != nodeid) {
			cost_matrix[globalMyID][nodeid] = cost;
			cost_matrix[nodeid][globalMyID] = cost;
		
			node_list[node_size] = nodeid;
			node_size = node_size + 1;
		}		
	}

	fclose(file);
	
	//printf("\nsocket() and bind() our socket. We will do all sendto()ing and recvfrom()ing on this one.0");
	if((globalSocketUDP=socket(AF_INET, SOCK_DGRAM, 0)) < 0)
	{
		perror("socket");
		exit(1);
	}

	char myAddr[100];
	struct sockaddr_in bindAddr;
	sprintf(myAddr, "10.1.1.%d", globalMyID);	
	memset(&bindAddr, 0, sizeof(bindAddr));
	bindAddr.sin_family = AF_INET;
	bindAddr.sin_port = htons(7777);
	inet_pton(AF_INET, myAddr, &bindAddr.sin_addr);
	
	if(bind(globalSocketUDP, (struct sockaddr*)&bindAddr, sizeof(struct sockaddr_in)) < 0)
	{
		perror("bind");
		close(globalSocketUDP);
		exit(1);
	}
	
	//start threads... feel free to add your own, and to remove the provided ones.
	pthread_t announcerThread;
	pthread_create(&announcerThread, 0, announceToNeighbors, (void*)0);
	
	constructNetworkTopology(cost_matrix);	
	pthread_t dvThread;
	pthread_create(&dvThread, 0, invokeDVCalculation, (void*)0);

	//good luck, have fun!
	listenForNeighbors();
	
	
	
}
