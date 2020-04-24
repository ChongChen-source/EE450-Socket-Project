#ifndef CLIENT_H
#define CLIENT_H

#define AWS_TCP_PORT "34319"  // the TCP port client will be connecting to

#define LOCALHOST "127.0.0.1"

#define MAXDATASIZE 1024
#define BACKLOG 10  // how many pending connections queue will hold
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

// get sockaddr, IPv4 or IPv6:
void *get_in_addr(struct sockaddr *sa);

#endif