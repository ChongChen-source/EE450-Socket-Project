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
#include "serverC.h"

using namespace std;

/*
 * Using Dijkstra Algorithm to find out the shortest path beyween the source vertex and the destination vertex in the chosen map.
 * @param mapInfo   The MAP struct containing the information of the map for calculation
 * @return  The length of the shortest path
 */
float shortestPath(MAP &mapInfo) {
    float minLen = FLT_MAX;
    
    return minLen;
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
    /******************************************* Server C UDP Boot Up ************************************************/
// copy from Beej's-Guide --listener.c
	int sockfd;
	struct addrinfo hints, *servinfo, *p;
	int rv;
	int numbytes;
	struct sockaddr_storage their_addr;

	char buf[MAXDATASIZE];
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
// copy from Beej's Guide --end

    cout << "The Server C is up and running using UDP on port " << SERVER_C_UDP_PORT << endl;

    /******************************************* main accept() loop ************************************************/
    while (1) {
		// For calculation, receive data for calculation from AWS
		map<int, vector<int> > paths;

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
		// for (auto& n : paths[mapInfoRecvFromAWS.destiVertex]) {
		// 	resultsSendToAWS.path[index] = n;
        //     index++;
		// }
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
		if ((numbytes = sendto(sockfd, &resultsSendToAWS, MAXDATASIZE, 0, (struct sockaddr *)&their_addr, addr_len)) == -1) {
			perror("senderr: sendto");
			exit(1);
		}
		cout << "The Server C has finished sending the output to AWS" << endl;
	
	}

    return 0;
}