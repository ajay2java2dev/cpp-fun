/*
** client.c -- a stream socket client demo
*/

#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <errno.h>
#include <string.h>
#include <netdb.h>
#include <sys/types.h>
#include <netinet/in.h>
#include <sys/socket.h>
#include <netinet/tcp.h>
#include <arpa/inet.h>

#define PORT "3490" // the port client will be connecting to 

#define MAXDATASIZE 100 // max number of bytes we can get at once 

// get sockaddr, IPv4 or IPv6:
void *get_in_addr(struct sockaddr *sa)
{
	if (sa->sa_family == AF_INET) {
		return &(((struct sockaddr_in*)sa)->sin_addr);
	}

	return &(((struct sockaddr_in6*)sa)->sin6_addr);
}


int main(int argc, char *argv[])
{
	int sockfd, numbytes;  
	char buf[MAXDATASIZE];
	struct addrinfo hints, *servinfo, *p;
	int rv;
	char s[INET6_ADDRSTRLEN];
	char arg[100];
	struct hostent *hp;
	struct sockaddr_in addr;
	int on = 1, sock;

	if (argc != 2) {
	    fprintf(stderr,"usage: http_client url\n");
	    exit(1);
	}

	//1. validate http request + retrieve first half
	strcpy(arg,argv[1]);
	
	//protocol http validate
	if (strncmp(arg,"http",4)) {
		fprintf(stderr, "INVALIDPROTOCOL");
		exit(1);
	}

	char ip [100];
	int port = 80; //default
	char page[100];

	sscanf(arg,"http://%99[^:]:%99d/%99[^/n]", ip, &port, page);

	printf("\n%s",ip);
	printf("\n%d",port);
	printf("\n%s",page);

	memset(&hints, 0, sizeof hints);
	hints.ai_family = AF_UNSPEC;
	hints.ai_socktype = SOCK_STREAM;

	if ((hp = gethostbyname(ip)) == NULL) {
		herror("gethostbyname");
		exit(1);
	}

	// loop through all the results and connect to the first we can
	bcopy(hp->h_addr, &addr.sin_addr, hp->h_length);
	addr.sin_port = htons(port);
	addr.sin_family = AF_INET;
	sock = socket(PF_INET, SOCK_STREAM, IPPROTO_TCP);
	setsockopt(sock, IPPROTO_TCP, TCP_NODELAY, (const char *)&on, sizeof(int));

	close(sockfd);

	return 0;
}

