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
#include "sha256.h"
#include "ftp.h"

#define BUFF 8192

//connects client and server
int connectServer(char* ip, int port) {
	printf("IP: %s\n", ip);
	printf("port: %d\n", port);

	int sock = 0;
	struct sockaddr_in serverAddr;

	sock = socket(AF_INET, SOCK_STREAM, 0);

	if(sock < 0) {
		fprintf(stderr, "Error. Socket creation failed.\n");
		exit(0);
	} else {
		printf("Socket creation is successful!\n");
	}

	memset(&serverAddr, '0', sizeof(serverAddr));
	serverAddr.sin_family = AF_INET;
	serverAddr.sin_addr.s_addr = inet_addr(ip);
	serverAddr.sin_port = htons(port);



	if(connect(sock, (struct sockaddr*) &serverAddr, sizeof(serverAddr)) < 0) {
		fprintf(stderr, "Error. Connection to server failed.\n");
		exit(0);
	} else {
		printf("Successfully connected to server!\n");
	}

	return sock;
}

void configure(char* ip, char* port) {
	int intPort = strtol(port, NULL, 10);
	if(intPort < 8000 || intPort > 65535) {
		fprintf(stderr, "Invalid port. Need to be >8k and <64k.\nTerminating.\n");
		exit(0);
	}
	int fd;

	fd = open(".configure", O_CREAT | O_RDWR, 0644);
	write(fd, ip, strlen(ip));
	write(fd, " ", 1);
	write(fd, port, strlen(port));
	close(fd);


}

char** readConfig() {
	int fd = open(".configure", O_RDONLY, 0644);
	if(fd == -1) {
		fprintf(stderr, "Cannot communicate with server. IP and Port is not configured.\nTerminating.\n");
		exit(0);
	}

	//get number of characters in text file
	int count = 0;
	char c;
	while(read(fd, &c, 1)) {
		count += 1;
	}
	close(fd);

	//gets textfile and puts it to string
	char* configStr = (char*) malloc(count * sizeof(char));
	fd = open(".configure", O_RDONLY, 0644);
	int i = 0;
	while(read(fd, &c, 1)) {
		configStr[i] = c;
		i++;
	}
	close(fd);

	//tokenize string to array
	char* token;
	token = strtok(configStr, " ");
	char** config2d = (char**) malloc(2 * sizeof(char*));

	config2d[0] = (char*) malloc(strlen(token) * sizeof(char));
	config2d[0] = token;

	token = strtok(NULL, " ");
	config2d[1] = (char*) malloc(strlen(token) * sizeof(char));
	config2d[1] = token;

	printf("IP: %s\n", config2d[0]);
	printf("Port: %s\n", config2d[1]);

	return config2d;


}

void create(char* projectName) {
	char** config = readConfig();
	int sockfd = connectServer(config[0], strtol(config[1], NULL, 10));


	//send the message line to the server
	int n = write(sockfd, "create ", strlen("create "));
	if (n < 0){
		fprintf(stderr, "Error. Cannot write to socket\n");
		exit(0);
	}

	n = write(sockfd, projectName, strlen(projectName));
	if (n < 0){
		fprintf(stderr, "Error. Cannot write to socket\n");
		exit(0);
	}

	char status[BUFF];
	bzero(status, BUFF);
	read(sockfd, status, BUFF);

	printf("status: %s\n\n", status);

	if(strcmp(status, "success") == 0) {
		printf("Project directory succesfully made on server! Creating directories locally...\n");

		char* filePath = (char*) malloc(strlen(projectName) * sizeof(char));
		filePath = projectName;
		mkdir(filePath, 0700);

		//TODO: Receive FTP from server here. Remove this part later...
		sprintf(filePath, "%s/.Manifest", filePath);
		int fd = open(filePath, O_CREAT | O_RDWR, 0644);
		printf("test\n");
		close(fd);

	} else {
		fprintf(stderr, "Error. Project with name already exists!\n");
		exit(0);
	}

	close(sockfd);
}


int main(int argc, char** argv) {
	char* ip;
	int port;



	recvOne("blah", 0);


	if(argc < 3) {
		fprintf(stderr, "Error. Invalid number of inputs.\n");
		return 0; 
	}

	if(strcmp("configure", argv[1]) == 0) {
		if(argc < 4) {
			fprintf(stderr, "Error. Invalid number of inputs for configure.\n");
			return 0;
		}
		configure(argv[2], argv[3]);
		printf("Configured!\nIP: %s\nPort: %s\n", argv[2], argv[3]);

	} else if (strcmp("checkout", argv[1]) == 0) {
		if(access(".configure", F_OK) != -1) {
			printf("file exists\n");
		} else {
			fprintf(stderr, "Error. Configure file does not exist. No valid IP and port.\n");
			return 0;
		}
	} else if (strcmp("create", argv[1]) == 0) {
		printf("create\n");
		create(argv[2]);

	}
	return 0;



}
