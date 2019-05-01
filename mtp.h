#include <stdlib.h>
#include <stdio.h>
#include <signal.h>
#include <sys/types.h>
#include <string.h>
#include <sys/socket.h>
#include <netdb.h>
#include <dirent.h>
#include <fcntl.h>
#include <sys/stat.h>
#include "zlib.h"




#define BUFF 8192

void sendMsg(char* msg, int sockfd) {
	int datalen = strlen(msg);
	int temp = htonl(datalen);
	int n = write(sockfd, (char*)&temp, sizeof(temp));
	if (n < 0){
		fprintf(stderr, "Error. Cannot write to socket\n");
		exit(0);
	}
	n = write(sockfd, msg, datalen);
	if (n < 0){
		fprintf(stderr, "Error. Cannot write to socket\n");
		exit(0);
	}
}

void recMsg(char msg[BUFF], int sockfd) {
	int buflen;
	int n = read(sockfd, (char*)&buflen, sizeof(buflen));
	if (n < 0){
		fprintf(stderr, "Error. Cannot read from socket");
		exit(0);
	}
	buflen = ntohl(buflen);
	n = read(sockfd, msg, buflen);
	if (n < 0){
		fprintf(stderr, "Error. Cannot read from socket");
		exit(0);
	}

}


void sendCompress(char* filePath, int sockfd) {
	gzFile fd = gzopen(filePath, "rb");
	char buffer[BUFF];
	while(gzread(fd, buffer, BUFF) > 0) {
	       sprintf(buffer, "%s", buffer);
	}	       

	sendMsg(buffer, sockfd);
}
