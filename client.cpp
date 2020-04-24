#include "client.h"
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

using namespace std;

// copy from Beej's Guide
void *get_in_addr(struct sockaddr *sa) {
  if (sa->sa_family == AF_INET) {
    return &(((struct sockaddr_in *)sa)->sin_addr);
  }

  return &(((struct sockaddr_in6 *)sa)->sin6_addr);
}

int main(int argc, char *argv[]) {
  // copy from Beej's Guide --begin
  int sockfd, numbytes;
  QUERY sendBuf;
  RESULT recvBuf;
  char buf[MAXDATASIZE];

  struct addrinfo hints, *servinfo, *p;
  int rv;
  char s[INET6_ADDRSTRLEN];

  if (argc == 5 && strcmp(argv[0], "./client") == 0)
    ;
  else {
    exit(1);
  }

  // store input parameters
  sendBuf.mapID = *argv[1];
  sendBuf.sourceVertex = atoi(argv[2]);
  sendBuf.destiVertex = atoi(argv[3]);
  sendBuf.fileSize = atoi(argv[4]);
  memset(&hints, 0, sizeof hints);
  hints.ai_socktype = SOCK_STREAM;

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

  inet_ntop(p->ai_family, get_in_addr((struct sockaddr *)p->ai_addr), s,
            sizeof s);
  cout << "The client is up and running" << endl;

  freeaddrinfo(servinfo);
  // copy from Beej's Guide --end

  // Client sends query to the AWS via TCP
  if (send(sockfd, &sendBuf, sizeof(sendBuf), 0) == -1)
    perror("send");

  cout << "The client has sent query to AWS using TCP:"
       << " start vertex " << sendBuf.sourceVertex << " ; destination vertex "
       << sendBuf.destiVertex << " ; map  " << sendBuf.mapID << " ; file size "
       << sendBuf.fileSize << endl;

  // Client receives the calculated results from AWS via TCP
  if ((numbytes = recv(sockfd, &buf, 1024, 0)) == -1) {
    perror("recv");
    exit(1);
  }
  memset(&recvBuf, 0, sizeof(recvBuf));
  memcpy(&recvBuf, buf, sizeof(recvBuf));

  // Error: No such map id found
  if (recvBuf.sourceVertex == -1) {
    cout << "No map id " << sendBuf.mapID << "> found" << endl;
  }

  // Error: No such source vertex index found
  else if (recvBuf.sourceVertex == -2) {
    cout << "No vertex id " << sendBuf.sourceVertex << "> found" << endl;
  }

  // Error: No such destination vertex index found
  else if (recvBuf.sourceVertex == -3) {
    cout << "No vertex id " << sendBuf.destiVertex << "> found" << endl;
  }

  // Dispalying received calculated results:
  else {
    cout << "The client has received results from AWS:" << endl;
    cout << "------------------------------------------------------" << endl;
    cout << "Source    Destination    Min Length      Tt      Tp      Delay    "
         << endl;
    cout.width(5);
    cout << recvBuf.sourceVertex;
    cout.width();
    cout << "     ";
    cout.width(6);
    cout << recvBuf.destiVertex;
    cout.width();
    cout << "         ";
    cout.width(8);
    cout << recvBuf.minLen;
    cout.width();
    cout << "      ";
    cout.width(6);
    cout << recvBuf.timeTrans;
    cout.width();
    cout << "  ";
    cout.width(6);
    cout << recvBuf.timeProp;
    cout.width();
    cout << "  ";
    cout.width(7);
    cout << recvBuf.delay;
    cout.width();
    cout << "    " << endl;
    cout << "------------------------------------------------------" << endl;
    cout << "Shortest path: ";
    for (int i = 0; i < SIZE; i++) {
      if (recvBuf.path[i] != -1) {
        cout << recvBuf.path[i] << " -- ";
      } else {
        break;
      }
    }
    cout << recvBuf.destiVertex << endl;
  }

  return 0;
}