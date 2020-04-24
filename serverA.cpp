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
#include "serverA.h"

using namespace std;

/* Read the information of maps stored in the file
 * @param fileName	The name of the file to read
 * @return a "map" data structure. The key is a map's ID, and the value is a MAP structure storing the map's information
 */
map<char, MAP> readMap (string fileName);

int main(void) {
	map<char, MAP> maps = readMap("map1.txt");

// copy from Beej's-Guide --listener.c
	int sockfd;
	struct addrinfo hints, *servinfo, *p;
	int rv;
	int numbytes;
	struct sockaddr_storage their_addr;

	char buf[MAXDATASIZE];
	char mapID_quired;
	MAP mapInfoToAWS;

	socklen_t addr_len;
	char s[INET6_ADDRSTRLEN];

	memset(&hints, 0, sizeof hints);
	hints.ai_family = AF_UNSPEC;
	hints.ai_socktype = SOCK_DGRAM;
	hints.ai_flags = AI_PASSIVE;

	if ((rv = getaddrinfo(LOCALHOST, SERVER_A_UDP_PORT, &hints, &servinfo)) != 0) {
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

	cout << "The Server A is up and running using UDP on port " << SERVER_A_UDP_PORT << endl;

	//main loop: 
	while (1) {
		// Server A receives the quired map ID form AWS via UDP

		// Server A looks up the map ID and return the map's information to AWS via UDP
	}

	return 0;
}