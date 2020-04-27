#include <arpa/inet.h>
#include <ctype.h>
#include <errno.h>
#include <fstream>
#include <iostream>
#include <map>
#include <math.h>
#include <netdb.h>
#include <netinet/in.h>
#include <set>
#include <stack>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <string>
#include <sys/socket.h>
#include <sys/types.h>
#include <sys/wait.h>
#include <unistd.h>
#include <vector>

#define AWS_TCP_PORT "34319"  // the TCP port client will be connecting to

#define LOCALHOST "127.0.0.1"

#define MAX_LEN 1024
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

using namespace std;

void *get_in_addr(struct sockaddr *sa) {
  if (sa->sa_family == AF_INET) {
    return &(((struct sockaddr_in *)sa)->sin_addr);
  }

  return &(((struct sockaddr_in6 *)sa)->sin6_addr);
}

int main(int argc, char *argv[]) {

// start code from Beej's Guide to Network Programming. http://www.beej.us/guide/bgnet/ -- start
  int sockfd, numbytes;
  QUERY queryBuf;
  RESULT resultBuf;
  char buf[MAX_LEN];

  struct addrinfo hints, *servinfo, *p;
  int rv;
  char s[INET6_ADDRSTRLEN];

  if (argc == 5 && strcmp(argv[0], "./client") == 0);
  else {
    exit(1);
  }
// start code from Beej's Guide to Network Programming. http://www.beej.us/guide/bgnet/ -- end
  // store input parameters
  queryBuf.mapID = *argv[1];
  queryBuf.sourceVertex = atoi(argv[2]);
  queryBuf.destiVertex = atoi(argv[3]);
  queryBuf.fileSize = atoi(argv[4]);
  memset(&hints, 0, sizeof hints);
  hints.ai_socktype = SOCK_STREAM;

// start code from Beej's Guide to Network Programming. http://www.beej.us/guide/bgnet/ -- start
  // Initialize TCP connection
  if ((rv = getaddrinfo(LOCALHOST, AWS_TCP_PORT, &hints, &servinfo)) != 0) {
    fprintf(stderr, "getaddrinfo: %s\n", gai_strerror(rv));
    return 1;
  }

  for (p = servinfo; p != NULL; p = p->ai_next) {
    if ((sockfd = socket(p->ai_family, p->ai_socktype, p->ai_protocol)) == -1) {
      perror("client: socket");
      continue;
    }
    if (connect(sockfd, p->ai_addr, p->ai_addrlen) == -1) {
      perror("client: connect");
      close(sockfd);
      continue;
    }
    break;
  }

  if (p == NULL) {
    fprintf(stderr, "client: failed to connect\n");
    return 2;
  }

  inet_ntop(p->ai_family, get_in_addr((struct sockaddr *)p->ai_addr), s, sizeof s);
  cout << "The client is up and running" << endl;
  freeaddrinfo(servinfo);
// start code from Beej's Guide to Network Programming. http://www.beej.us/guide/bgnet/ -- start

  // Client sends query to the AWS via TCP
  if (send(sockfd, &queryBuf, sizeof(queryBuf), 0) == -1) {
    perror("send");
  }

  cout << "The client has sent query to AWS using TCP:"
       << " start vertex " << queryBuf.sourceVertex << " ; destination vertex "
       << queryBuf.destiVertex << " ; map  " << queryBuf.mapID << " ; file size "
       << queryBuf.fileSize << endl;

  // Client receives the calculated results from AWS via TCP
  if ((numbytes = recv(sockfd, &buf, 1024, 0)) == -1) {
    perror("recv");
    exit(1);
  }
  memset(&resultBuf, 0, sizeof(resultBuf));
  memcpy(&resultBuf, buf, sizeof(resultBuf));

  // Error: No such map id found
  if (resultBuf.sourceVertex == -1) {
    cout << "No map id " << queryBuf.mapID << "> found" << endl;
  }

  // Error: No such source vertex index found
  else if (resultBuf.sourceVertex == -2) {
    cout << "No vertex id " << queryBuf.sourceVertex << "> found" << endl;
  }

  // Error: No such destination vertex index found
  else if (resultBuf.sourceVertex == -3) {
    cout << "No vertex id " << queryBuf.destiVertex << "> found" << endl;
  }

  // Dispalying received calculated results:
  else {
    printf("The client has received results form AWS:\n");
    printf("-------------------------------------------------------\n");
    printf("Source   Destination    Min Length    Tt    Tp    Delay\n");
    printf("-------------------------------------------------------\n");
    printf("  %d        %d            %.2f         %.2f  %.2f   %.2f\n",
      resultBuf.sourceVertex, resultBuf.destiVertex, resultBuf.minLen, resultBuf.timeTrans, resultBuf.timeProp, resultBuf.delay);
    printf("-------------------------------------------------------\n");
    for (int i = 0; i < SIZE; i++) {
      if (resultBuf.path[i] != -1) {
        cout << resultBuf.path[i] << " -- ";
      } else {
        break;
      }
    }
    cout << resultBuf.destiVertex << endl;
  }

  return 0;
}