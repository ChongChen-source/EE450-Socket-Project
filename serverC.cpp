#include <sys/socket.h> 
#include <sys/wait.h>
#include <iostream>
#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <errno.h>
#include <string.h>
#include <string>
#include <sys/types.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <netdb.h>
#include <vector>
#include <fstream>
#include <ctype.h>
#include <math.h>
#include <set>
#include <map>
#include <stack>
#include <float.h>

#define SERVER_C_UDP_PORT "32319"
#define AWS_UDP_PORT "33319"

#define LOCALHOST "127.0.0.1"

#define MAX_LEN 1024
#define BACKLOG 10  // how many pending connections queue will hold
#define SIZE 10     // For each map, the maximum number of vertices is 10

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

using namespace std;

int vertices[SIZE];
float distances[SIZE][SIZE];
map<int, vector<int> > paths;

/*
 * Using Dijkstra Algorithm to find out the shortest path beyween the source vertex and the destination vertex in the chosen map.
 * @param mapInfo   The MAP struct containing the information of the map for calculation
 * @return  The length of the shortest path
 * 
 * Modified form https://www.geeksforgeeks.org/dijkstras-shortest-path-algorithm-greedy-algo-7/
 */
float shortestPath(MAP &mapInfo) {
	int sour = mapInfo.sourceVertex;
	int dest = mapInfo.destiVertex;
	memcpy(vertices,mapInfo.vertices,sizeof(mapInfo.vertices));
	memcpy(distances,mapInfo.distances,sizeof(mapInfo.distances));

	set<int> visitedVertices;
	int curr;
	for (int i = 0; i < SIZE; i++) {
		if (vertices[i] != -1) paths[vertices[i]].push_back(sour);
		if (vertices[i] == sour) curr = i;
	}

	visitedVertices.insert(sour);
	for (int i = 1; i < SIZE; i += 1) {
		double min = MAXFLOAT;
		int index = 0;
		for (int j = 0; j < SIZE; j++) {
			if (visitedVertices.find(j) == visitedVertices.end() && vertices[j] != -1 && *((float*)distances + SIZE * curr + j) < min) {
				min = *((float*)distances + SIZE * sour + j);
				index = j;
			}
		}
		if (min == MAXFLOAT) continue;
		visitedVertices.insert(index);

		for (int j = 0; j < SIZE; j++) {
			if (*((float*)distances +  SIZE * curr + j) > *((float*)distances + SIZE * curr + index) + *((float*)distances + SIZE * index + j)) {
				paths[vertices[j]] = paths[vertices[index]];
				paths[vertices[j]].push_back(vertices[index]);
				*((float*)distances + SIZE * curr + j) = *((float*)distances + SIZE * curr+ index) + *((float*)distances + SIZE * index + j);
			}
		}
	}

	int loc = -1;
	for (int i = 0; i < SIZE; i++) {
		if (vertices[i] == dest) loc = i;
	}

	return *((float*)distances + SIZE * curr + loc);
}

// get sockaddr, IPv4 or IPv6:
void *get_in_addr(struct sockaddr *sa)
{
	if (sa->sa_family == AF_INET) {
		return &(((struct sockaddr_in*)sa)->sin_addr);
	}
	return &(((struct sockaddr_in6*)sa)->sin6_addr);
}

int main(void) {

// start code from Beej's Guide to Network Programming. http://www.beej.us/guide/bgnet/ -- start
/******************************************* Server C UDP Boot Up ************************************************/
	int sockfd;
	struct addrinfo hints, *servinfo, *p;
	int rv;
	int numbytes;
	struct sockaddr_storage their_addr;

	char buf[MAX_LEN];
	MAP mapInfoRecvFromAWS;
	RESULT resultsSendToAWS;

	socklen_t addr_len;
	char s[INET6_ADDRSTRLEN];

	memset(&hints, 0, sizeof hints);
	hints.ai_family = AF_UNSPEC;
	hints.ai_socktype = SOCK_DGRAM;
	hints.ai_flags = AI_PASSIVE;

	if ((rv = getaddrinfo(LOCALHOST, SERVER_C_UDP_PORT, &hints, &servinfo)) != 0) {
		fprintf(stderr, "getaddrinfo: %s\n", gai_strerror(rv));
		return 1;
	}

	for (p = servinfo; p != NULL; p = p->ai_next) {
		if ((sockfd = socket(p->ai_family, p->ai_socktype,
			p->ai_protocol)) == -1) {
			perror("listener: socket");
			continue;
		}
		if (::bind(sockfd, p->ai_addr, p->ai_addrlen) == -1) {
			close(sockfd);
			perror("listener: bind");
			continue;
		}
		break;
	}
	if (p == NULL) {
		fprintf(stderr, "listener: failed to bind socket\n");
		return 2;
	}
	freeaddrinfo(servinfo);
// start code from Beej's Guide to Network Programming. http://www.beej.us/guide/bgnet/ -- end

    cout << "The Server C is up and running using UDP on port " << SERVER_C_UDP_PORT << endl;

/************************************************ main loop ******************************************************/
    while (1) {
		// For calculation, receive data for calculation from AWS
		addr_len = sizeof their_addr;
		if ((numbytes = recvfrom(sockfd, &buf, 1024, 0, (struct sockaddr *)&their_addr, &addr_len)) == -1) {
			perror("recvfrom");
			exit(1);
		}

		memset(&mapInfoRecvFromAWS, 0, sizeof(mapInfoRecvFromAWS));
		memcpy(&mapInfoRecvFromAWS, buf, sizeof(mapInfoRecvFromAWS));

		cout << "The Server C has received data for calculation:" << endl;
		cout << "* Propagation speed: " << mapInfoRecvFromAWS.speedProp << " km/s;" << endl;
		cout << "* Transmission speed: " << mapInfoRecvFromAWS.speedTrans << " KB/s;" << endl;
        cout << "map ID: " << mapInfoRecvFromAWS.mapID << endl;
		cout << "* Source ID: " << mapInfoRecvFromAWS.sourceVertex
             << "    Destination ID: " << mapInfoRecvFromAWS.destiVertex << endl;

        // Calculation
		resultsSendToAWS.minLen = shortestPath(mapInfoRecvFromAWS);
		resultsSendToAWS.sourceVertex = mapInfoRecvFromAWS.sourceVertex;
		resultsSendToAWS.destiVertex = mapInfoRecvFromAWS.destiVertex;

		resultsSendToAWS.timeProp = resultsSendToAWS.minLen / mapInfoRecvFromAWS.speedProp;
		resultsSendToAWS.timeTrans = (float)mapInfoRecvFromAWS.fileSize / (float)mapInfoRecvFromAWS.speedTrans;
		resultsSendToAWS.delay = resultsSendToAWS.timeProp + resultsSendToAWS.timeTrans;

		int index = 0;

		for (auto& n : paths[mapInfoRecvFromAWS.destiVertex]) {
			resultsSendToAWS.path[index] = n;
            index++;
		}

		while (index < SIZE) {
			resultsSendToAWS.path[index] = -1;
            index++;
		}

        // After calculation
		cout << "The Server C has finished the calculation:" << endl;
		cout << "Shortest path: ";
		for (int i = 0; i < SIZE; i++) {
			if (resultsSendToAWS.path[i] != -1) {
				cout << resultsSendToAWS.path[i] << " -- ";
			} else {
                break;
            }
		}
		cout << resultsSendToAWS.destiVertex << endl;
		cout << "Shortest distance: <" << resultsSendToAWS.minLen << " km" << endl;
		cout << "Transmission delay: <" << resultsSendToAWS.timeTrans << " s" << endl;
		cout << "Propagation delay: <" << resultsSendToAWS.timeProp << " s" << endl;
		
        // Send results to the AWS server
		if ((numbytes = sendto(sockfd, &resultsSendToAWS, MAX_LEN, 0, (struct sockaddr *)&their_addr, addr_len)) == -1) {
			perror("senderr: sendto");
			exit(1);
		}
		cout << "The Server C has finished sending the output to AWS" << endl;
	
	}

    return 0;
}