#ifndef SERVERC_H
#define SERVERC_H

#define SERVER_C_UDP_PORT "32319"
#define AWS_UDP_PORT "33319"

#define LOCALHOST "127.0.0.1"

#define MAXDATASIZE 1024
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
    int sourceVertex;   // Vertices index is between 0 and 99
    int destiVertex;    // Vertices index is between 0 and 99
    int fileSize;       // Unit: KB, [1, 1048576)
    float speedProp;    // Unit: km/s, [10000.00, 299792.00)
    int speedTrans;     // Unit: KB/s, [1, 1048576)
    int vertices[SIZE];
    //the max number of edges is 40
    float distances[SIZE][SIZE]; // Unit: km, [1.00, 10000.00)
}MAP;

// get sockaddr, IPv4 or IPv6:
void *get_in_addr(struct sockaddr *sa);

#endif