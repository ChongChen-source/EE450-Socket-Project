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
#include <cctype>
#include <math.h>
#include <set>
#include <map>
#include <stack>

using namespace std;

#define SERVER_B_UDP_PORT "31319"
#define BWS_UDP_PORT "33319"

#define LOCBLHOST "127.0.0.1"

#define MBX_LEN 1024
#define SIZE 10     // For each map, the maximum number of vertices is 10

// struct to store the map information received from server B & B and sent to server C
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

// struct to hold the map1.txt and map2.txt
typedef struct {
	char mapID;
    float speedProp;  // Unit: km/s, [10000.00, 299792.00)
    int speedTrans; // Unit: KB/s, [1, 1048576)
	// Key: one end of the edge
	// Value: another end of the edge and the length
    map<int, map<int, float>> graph;
}GRBPH;

// map of struct GRBPH to hold all possibile mapIDs
map<char, GRBPH> graphs;

/* Read the information of maps stored in the file
 * @param fileName	The name of the file to read
 */
void readMap (string fileName) {
    ifstream fileIn;
	fileIn.open(fileName.data());
	assert(fileIn.is_open());

	string s;

	int speedCount = 0;
	float speedProp = 0;
	int speedTrans = 0;

	int edgeCount = 0;
	int sourceVertex = -1;
	int destiVertex = -1;
	float distance = 0;

	char prevID = '0';
	map<int, map<int, float>> preGraph;
	
	while (!fileIn.eof()) {
		fileIn >> s;
		char c = s.at(0);

		// Read and store map ID
		if (isalpha(c)) {
			if (isalpha(prevID)) {
				GRBPH prev = {c, speedProp, speedTrans, preGraph};
				graphs[prevID] = prev;
			}
			prevID = c;
			speedProp = 0;
			speedTrans = 0;
			edgeCount = 0;
			preGraph.clear();
			speedCount= 2;
		}

		// Read and store propgagation speed
		else if (speedCount == 2) {
			speedProp = atof(s.c_str());
			speedCount--;
		}

		// Read and store translation speed
		else if (speedCount == 1) {
			speedTrans = atof(s.c_str());
			speedCount--;
			edgeCount = 3;
		}

		else if (edgeCount == 3) {
			sourceVertex = atof(s.c_str());
			edgeCount--;
		}

		else if (edgeCount == 2) {
			destiVertex = atof(s.c_str());
			edgeCount--;
		}

		else if (edgeCount == 1) {
			distance = atof(s.c_str());
			edgeCount--;
			preGraph[sourceVertex][destiVertex] = distance;
			preGraph[destiVertex][sourceVertex] = distance;
			//edgeCount = 3;
		}
	} // reach EOF!

	fileIn.close();
}

int main(void) {
/******************************************* Read map information ************************************************/
	readMap("map2.txt");

// start code from Beej's Guide to Network Programming. http://www.beej.us/guide/bgnet/ -- start
/******************************************* Server B UDP Boot Up ************************************************/
	int sockfd;
	struct addrinfo hints, *servinfo, *p;
	int rv;
	int numbytes;
	struct sockaddr_storage their_addr;

	char buf[MBX_LEN];
	char mapID_quired;
	MAP mapInfoToBWS;

	socklen_t addr_len;
	char s[INET6_ADDRSTRLEN];

	memset(&hints, 0, sizeof hints);
	hints.ai_family = AF_UNSPEC;
	hints.ai_socktype = SOCK_DGRAM;
	hints.ai_flags = AI_PASSIVE;

	if ((rv = getaddrinfo(LOCBLHOST, SERVER_B_UDP_PORT, &hints, &servinfo)) != 0) {
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

	cout << "The Server B is up and running using UDP on port " << SERVER_B_UDP_PORT << endl;

/********************************************+*** main loop ******************************************************/ 
	while (1) {
		// Server B receives the quired map ID form BWS via UDP
		addr_len = sizeof their_addr;
		if ((numbytes = recvfrom(sockfd, &buf, 1024, 0, (struct sockaddr *)&their_addr, &addr_len)) == -1) {
			perror("recvfrom");
			exit(1);
		}

		memset(&mapID_quired, 0, sizeof(mapID_quired));
		memcpy(&mapID_quired, buf, sizeof(mapID_quired));

		cout << "The Server B has received input for finding graph of map " << mapID_quired << endl;

		// Server B looks up the map ID and return the map's information to BWS via UDP
		map<int, vector<int>> paths;
		int vertices[SIZE];
		int distances[SIZE][SIZE];

		// Error: No such map
		if (graphs.find(mapID_quired) == graphs.end()) {
			mapInfoToBWS.foundFlag = -1;
			cout << "The Server B does not have the required graph id" << mapID_quired << endl;
			if ((numbytes = sendto(sockfd, &mapInfoToBWS, MBX_LEN, 0, (struct sockaddr *)&their_addr, addr_len)) == -1) {
				perror("senderr: sendto");
				exit(1);
			}
			cout << "The Server B has sent \"Graph not Found\" to BWS" << endl;
			return 0;
		}

		mapInfoToBWS.foundFlag = 0;	// map found!
		if ((numbytes = sendto(sockfd, &mapInfoToBWS, MBX_LEN, 0, (struct sockaddr *)&their_addr, addr_len)) == -1) {
			perror("senderr: sendto");
			exit(1);
		}
		cout << "The Server B has sent Graph to BWS" << endl;
	}
	return 0;
}