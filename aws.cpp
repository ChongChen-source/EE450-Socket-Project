#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <errno.h>
#include <string.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <netdb.h>
#include <arpa/inet.h>
#include <sys/wait.h>
#include <signal.h>
#include <iostream>

using namespace std;

#define SERVER_A_UDP_PORT "30319"
#define SERVER_B_UDP_PORT "31319"
#define SERVER_C_UDP_PORT "32319"
#define AWS_UDP_PORT "33319"
#define AWS_TCP_PORT "34319"  // the TCP port client will be connecting to

#define LOCALHOST "127.0.0.1"

#define BACKLOG 10  // how many pending connections queue will hold
#define MAX_LEN 1024
#define SIZE 10     // For each map, the maximum number of vertices is 10

// struct to store the query information received from the client
typedef struct {
    char mapID;         
    int sourceVertex;   // Vertices index is between 0 and 99
    int destiVertex;    // Vertices index is between 0 and 99
    int fileSize;       // Unit: KB, [1, 1048576)
}QUERY;

// struct to store the result information sent to the clinet
typedef struct {
    int sourceVertex;   // Vertices index is between 0 and 99
    int destiVertex;    // Vertices index is between 0 and 99
    float minLen;
    float timeTrans;
    float timeProp;
    float delay;
    int path[SIZE];
}RESULT;

// struct to store the map information received from server A & B and sent to server C
typedef struct {
    char mapID;
	int foundFlag;
    int sourceVertex;   // Vertices index is between 0 and 99
    int destiVertex;    // Vertices index is between 0 and 99
    int fileSize;       // Unit: KB, [1, 1048576)
    float speedProp;    // Unit: km/s, [10000.00, 299792.00)
    int speedTrans;     // Unit: KB/s, [1, 1048576)
    int vertices[SIZE];
    //the max number of edges is 40
    float distances[SIZE][SIZE]; // Unit: km, [1.00, 10000.00)
}MAP;

// start code from Beej's Guide to Network Programming. http://www.beej.us/guide/bgnet/ -- Start
void sigchld_handler(int s)
{
	int saved_errno = errno;
	while(waitpid(-1, NULL, WNOHANG) > 0);
	errno = saved_errno;
}
// get sockaddr, IPv4 or IPv6:
void *get_in_addr(struct sockaddr *sa)
{
	if (sa->sa_family == AF_INET) {
		return &(((struct sockaddr_in*)sa)->sin_addr);
	}
	return &(((struct sockaddr_in6*)sa)->sin6_addr);
}
// start code from Beej's Guide to Network Programming. http://www.beej.us/guide/bgnet/ -- end

int main(void)
{
// start code from Beej's Guide to Network Programming. http://www.beej.us/guide/bgnet/ -- start
	int sockfd, new_fd;  			
	struct addrinfo hints, *servinfo, *p;	
	struct sockaddr_storage their_addr; 	
	socklen_t sin_size;			
	struct sigaction sa;			
	int yes=1;	
	int i;
	int port_number;
	char s[INET6_ADDRSTRLEN];		
	int rv;					
	int numbytes;				
	char snd_buf[MAX_LEN];
// start code from Beej's Guide to Network Programming. http://www.beej.us/guide/bgnet/ -- end
	QUERY queryFromClient;
	MAP mapInfoFromA;
	MAP mapInfoFromB;
	MAP mapInfoSendToC;
	RESULT resultsRecvFromC;
	RESULT resultSendToClient;

// start code from Beej's Guide to Network Programming. http://www.beej.us/guide/bgnet/ -- start

/****************************************** AWS TCP Initialization ******************************************/ 
	memset(&hints, 0, sizeof hints);
	hints.ai_family = AF_UNSPEC;
	hints.ai_socktype = SOCK_STREAM;
	hints.ai_flags = AI_PASSIVE;

	if ((rv = getaddrinfo(LOCALHOST, AWS_TCP_PORT, &hints, &servinfo)) != 0) {
		fprintf(stderr, "getaddrinfo: %s\n", gai_strerror(rv));
		return 1;
	}
	
	for (p = servinfo; p != NULL; p = p->ai_next) {
		if ((sockfd = socket(p->ai_family, p->ai_socktype,
			p->ai_protocol)) == -1) {
			perror("server: socket");
			continue;
		}

		if (setsockopt(sockfd, SOL_SOCKET, SO_REUSEADDR, &yes,
			sizeof(int)) == -1) {
			perror("setsockopt");
			exit(1);
		}

		if (::bind(sockfd, p->ai_addr, p->ai_addrlen) == -1) {
			close(sockfd);
			perror("server: bind");
			continue;
		}

		break;
	}
	freeaddrinfo(servinfo);

	if (p == NULL) {
		fprintf(stderr, "server: failed to bind\n");
		exit(1);
	}
	if (listen(sockfd, BACKLOG) == -1) {
		perror("listen");
		exit(1);
	}
	sa.sa_handler = sigchld_handler;
	sigemptyset(&sa.sa_mask);
	sa.sa_flags = SA_RESTART;
	if (sigaction(SIGCHLD, &sa, NULL) == -1) {
		perror("sigaction");
		exit(1);
	}

/****************************************** AWS UDP Initialization ******************************************/
	int udp_sockfd;
	struct addrinfo udp_hints, *udp_servinfo, *udp_p;
	int udp_rv;
	struct sockaddr_storage udp_their_addr;
	char udp_buf[MAX_LEN];
	socklen_t udp_addr_len;
	char udp_s[INET6_ADDRSTRLEN];

	memset(&udp_hints, 0, sizeof udp_hints);
	udp_hints.ai_family = AF_UNSPEC;
	udp_hints.ai_socktype = SOCK_DGRAM;
	udp_hints.ai_flags = AI_PASSIVE;

	if ((udp_rv = getaddrinfo(LOCALHOST, AWS_UDP_PORT, &udp_hints, &udp_servinfo)) != 0) {
		fprintf(stderr, "getaddrinfo: %s\n", gai_strerror(udp_rv));
		return 1;
	}

	// loop through all the results and bind to the first we can (from Beej's-Guide: server.c)
	for (udp_p = udp_servinfo; udp_p != NULL; udp_p = udp_p->ai_next) {
		if ((udp_sockfd = socket(udp_p->ai_family, udp_p->ai_socktype,
			udp_p->ai_protocol)) == -1) {
			perror("listener: socket");
			continue;
		}

		if (::bind(udp_sockfd, udp_p->ai_addr, udp_p->ai_addrlen) == -1) {
			close(udp_sockfd);
			perror("listener: bind");
			continue;
		}

		break;
	}

	if (udp_p == NULL) {
		fprintf(stderr, "listener: failed to bind socket\n");
		return 2;
	}

	freeaddrinfo(udp_servinfo);

/******************************************* Server A Initialization *******************************************/
	struct addrinfo udp_A_hints, *udp_A_servinfo, *udp_A_p;
	int udp_A_rv;

	memset(&udp_A_hints, 0, sizeof udp_A_hints);
	udp_A_hints.ai_family = AF_UNSPEC;
	udp_A_hints.ai_socktype = SOCK_DGRAM;

	if ((udp_A_rv = getaddrinfo(LOCALHOST, SERVER_A_UDP_PORT, &udp_A_hints, &udp_A_servinfo)) != 0) {
		fprintf(stderr, "getaddrinfo: %s\n", gai_strerror(udp_A_rv));
		return 1;
	}

	udp_A_p = udp_A_servinfo;

/******************************************* Server B Initialization *******************************************/
	struct addrinfo udp_B_hints, *udp_B_servinfo, *udp_B_p;
	int udp_B_rv;

	memset(&udp_B_hints, 0, sizeof udp_B_hints);
	udp_B_hints.ai_family = AF_UNSPEC;
	udp_B_hints.ai_socktype = SOCK_DGRAM;

	if ((udp_B_rv = getaddrinfo(LOCALHOST, SERVER_B_UDP_PORT, &udp_B_hints, &udp_B_servinfo)) != 0) {
		fprintf(stderr, "getaddrinfo: %s\n", gai_strerror(udp_B_rv));
		return 1;
	}

	udp_B_p = udp_B_servinfo;

/******************************************* Server C Initialization *******************************************/
	struct addrinfo udp_C_hints, *udp_C_servinfo, *udp_C_p;
	int udp_C_rv;

	memset(&udp_C_hints, 0, sizeof udp_C_hints);
	udp_C_hints.ai_family = AF_UNSPEC;
	udp_C_hints.ai_socktype = SOCK_DGRAM;

	if ((udp_C_rv = getaddrinfo(LOCALHOST, SERVER_C_UDP_PORT, &udp_C_hints, &udp_C_servinfo)) != 0) {
		fprintf(stderr, "getaddrinfo: %s\n", gai_strerror(udp_C_rv));
		return 1;
	}

	udp_C_p = udp_C_servinfo;
// start code from Beej's Guide to Network Programming. http://www.beej.us/guide/bgnet/ -- end

/******************************************* AWS Boot Up Message ***********************************************/
	cout << "The AWS is up and running." << endl;
/******************************************* main accept() loop ************************************************/
	while(1) {
		sin_size = sizeof their_addr;
		new_fd = accept(sockfd, (struct sockaddr *)&their_addr, &sin_size);
		if (new_fd == -1) {
			perror("accept");
			continue;
		}

		// start code from Beej's Guide to Network Programming. http://www.beej.us/guide/bgnet/ -- start
		// get connection from the client
		inet_ntop(their_addr.ss_family,
			get_in_addr((struct sockaddr *)&their_addr),
			s, sizeof s);

		if (!fork()) { // this is the child process
			close(sockfd); // child doesn't need the listener

			// AWS receives the query information from clinet via TCP
			if ((numbytes = recv(new_fd, snd_buf, 1024, 0)) == -1) {
				perror("recv");
				exit(1);
			}
		// start code from Beej's Guide to Network Programming. http://www.beej.us/guide/bgnet/ -- end
			// store the client's query information
			memset(&queryFromClient, 0, sizeof(queryFromClient));
			memcpy(&queryFromClient, snd_buf, sizeof(queryFromClient));
			cout << "The AWS has received map ID " << queryFromClient.mapID 
				 << ", start vertex " << queryFromClient.sourceVertex 
				 << ", destination vertex " << queryFromClient.destiVertex
				 << " and file size " << queryFromClient.fileSize
				 << " from the client using TCP over port " << AWS_TCP_PORT
				 << endl;
	
			// AWS forwards the map ID to Server A via UDP
			char mapID_sendToA = queryFromClient.mapID;
			if ((numbytes = sendto(udp_sockfd, &mapID_sendToA, sizeof(mapID_sendToA), 0, udp_A_p->ai_addr, udp_A_p->ai_addrlen)) == -1) {
				perror("talker: sendto");
				exit(1);
			}
			cout << "The AWS has sent map ID to server A using UDP over port " << AWS_UDP_PORT << endl;

			// Server A seraches for the map ID from map1.txt

			// Server A sends the search result back to AWS via UDP
			memset(udp_buf, 0, 1024);
			udp_addr_len = sizeof udp_their_addr;
			if ((numbytes = recvfrom(udp_sockfd, &udp_buf, 1024, 0, (struct sockaddr *)&udp_their_addr, &udp_addr_len)) == -1) {
				perror("recvfrom");
				exit(1);
			}

			memset(&mapInfoFromA, 0, sizeof(mapInfoFromA));
			memcpy(&mapInfoFromA, udp_buf, sizeof(mapInfoFromA));

			// foundFlag = 0: Found map
			// foundFlag = -1: No such map
			// foundFlag = -2: No such source vertex
			// foundFlag = -3: No such destination vertex 

			if (mapInfoFromA.foundFlag == 0) {
				cout << "The AWS has received map information from server A" << endl;
			}

			// map not found by serverA in map1.txt
			if (mapInfoFromA.foundFlag == -1) {
				// AWS forwards the map ID to Server B via UDP
				char mapID_sendToB = queryFromClient.mapID;		

				if ((numbytes = sendto(udp_sockfd, &mapID_sendToB, sizeof(mapID_sendToB), 0, udp_B_p->ai_addr, udp_B_p->ai_addrlen)) == -1) {
					perror("talker: sendto");
					exit(1);
				}

				cout << "The AWS has sent map ID to server B using UDP over port " << AWS_UDP_PORT << endl;

				// receive result from B
				memset(udp_buf, 0, 1024);
				udp_addr_len = sizeof udp_their_addr;
				if ((numbytes = recvfrom(udp_sockfd, &udp_buf, 1024, 0, (struct sockaddr *)&udp_their_addr, &udp_addr_len)) == -1) {
					perror("recvfrom");
					exit(1);
				}

				// Server B seraches for the map ID from map1.txt

				// Server B sends the search result back to AWS via UDP
				memset(&mapInfoFromB, 0, sizeof(mapInfoFromB));
				memcpy(&mapInfoFromB, udp_buf, sizeof(mapInfoFromB));

				if (mapInfoFromB.foundFlag == 0) {
					cout << "The AWS has received map information from server B" << endl;
				}
			}

			// Receive map info, check source and destination vertices
			if (mapInfoFromA.foundFlag == 0) mapInfoSendToC = mapInfoFromA;
			else if (mapInfoFromB.foundFlag == 0) mapInfoSendToC = mapInfoFromB;
			int sour = mapInfoSendToC.sourceVertex;
			int dest = mapInfoSendToC.destiVertex;

			for (int vi = 0; vi < SIZE; vi++) {
				int vertexIndex = mapInfoSendToC.vertices[vi];
				if (vertexIndex == sour) break;
				if (vertexIndex == -1 || vi == SIZE) mapInfoSendToC.foundFlag = -2;
			}

			for (int vi = 0; vi < SIZE; vi++) {
				int vertexIndex = mapInfoSendToC.vertices[vi];
				if (vertexIndex == dest) break;
				if (vertexIndex == -1 || vi == SIZE) mapInfoSendToC.foundFlag = -3;
			}

			if (mapInfoSendToC.foundFlag == 0) {
				cout << "The source and destination vertex are in the graph" << endl;

				// AWS sends the map information to Server C via UDP
				if ((numbytes = sendto(udp_sockfd, &mapInfoSendToC, sizeof(mapInfoSendToC), 0, udp_C_p->ai_addr, udp_C_p->ai_addrlen)) == -1) {
					perror("talker: sendto");
					exit(1);
				}
				cout << "The AWS has sent map, source ID, destination ID, propagation speed and transmission speed to server C using UDP over port "
				     << AWS_UDP_PORT << endl;
				
				// Server C sends the result information back to AWS via UDP
				memset(udp_buf, 0, 1024);
				udp_addr_len = sizeof udp_their_addr;
				if ((numbytes = recvfrom(udp_sockfd, &udp_buf, 1024, 0, (struct sockaddr *)&udp_their_addr, &udp_addr_len)) == -1) {
					perror("recvfrom");
					exit(1);
				}

				memset(&resultsRecvFromC, 0, sizeof(resultsRecvFromC));
				memcpy(&resultsRecvFromC, udp_buf, sizeof(resultsRecvFromC));

				cout << "The AWS has received results from server C: " << endl; 
				cout << "Shortest path: ";
				for (int i = 0; i < SIZE; i++) {
					if (resultsRecvFromC.path[i] != -1) {
						cout << resultsRecvFromC.path[i] << " -- ";
					} else {
						break;
					}
				}
				cout << resultsRecvFromC.destiVertex << endl;
				cout << "Shortest distance: " << resultsRecvFromC.minLen << " km" << endl;
				cout << "Transmission delay: " << resultsRecvFromC.timeTrans << " s" << endl;
				cout << "Propagation delay: " << resultsRecvFromC.timeProp << " s" << endl;
				resultSendToClient = resultsRecvFromC;
			}

			close(udp_sockfd);

			// map not found neither by serverA or serverB
			if (mapInfoFromA.foundFlag == -1 && mapInfoFromB.foundFlag == -1) {
				resultSendToClient.sourceVertex = -1;
			}

			// Source vertex not found
			else if (mapInfoSendToC.foundFlag == -2) {
				resultSendToClient.sourceVertex = -2;
				cout << "Source vertex not found in the graph, sending error to client using TCP over port " << AWS_TCP_PORT << endl;
			}

			// Source vertex not found
			else if (mapInfoSendToC.foundFlag == -3) {
				resultSendToClient.sourceVertex = -3;
				cout << "Destination vertex not found in the graph, sending error to client using TCP over port " << AWS_TCP_PORT << endl;
			}

			// AWS sends to client the shortest path and delay results
			if (send(new_fd, &resultSendToClient, sizeof(resultSendToClient), 0) == -1)
				perror("send");
			else
				cout << "The AWS has sent calculated results to client using TCP over port " << AWS_TCP_PORT << endl;
			close(new_fd);
			exit(0);
		}
		close(new_fd);  // parent doesn't need this
	}

	return 0;
}
