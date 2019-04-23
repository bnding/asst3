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


	//TODO: make this another method. 
	//for command
	int datalen = strlen("create");
	int temp = htonl(datalen);
	int n = write(sockfd, (char*)&temp, sizeof(temp));
	if (n < 0){
		fprintf(stderr, "Error. Cannot write to socket\n");
		exit(0);
	}
	n = write(sockfd, "create", datalen);
	if (n < 0){
		fprintf(stderr, "Error. Cannot write to socket\n");
		exit(0);
	}


	//for project name
	datalen = strlen(projectName);
	temp = htonl(datalen);
	n = write(sockfd, (char*)&temp, sizeof(temp));
	if (n < 0){
		fprintf(stderr, "Error. Cannot write to socket\n");
		exit(0);
	}
	n = write(sockfd, projectName , datalen);
	if (n < 0){
		fprintf(stderr, "Error. Cannot write to socket\n");
		exit(0);
	}

	char file[BUFF];
	bzero(file, BUFF);
	read(sockfd, file, BUFF);

	printf("status: %s\n\n", file);

	if(strlen(file) != 0) {
		printf("Project directory succesfully made on server! Creating directories locally...\n");

		char* filePath = (char*) malloc(strlen(projectName) * sizeof(char));
		filePath = projectName;
		mkdir(filePath, 0700);

		char** recFile = decodeFile(projectName, file);

		sprintf(filePath, "%s%s", filePath, recFile[0]);
		int fd = open(filePath, O_CREAT | O_RDWR, 0644);
		close(fd);

	} else {
		fprintf(stderr, "Error. Project with name already exists!\n");
		exit(0);
	}

	close(sockfd);
}


int folderExist(char* lookingFor){
	struct stat buffer;
	stat(lookingFor, &buffer);

	if (S_ISDIR(buffer.st_mode)){
		return 1;
	}
	return 0;

}


void checkout(char* projectName) {
	//TODO zip file from server to client. Need to first check if project exists at the server.
	
	char** config = readConfig();
	int sockfd = connectServer(config[0], strtol(config[1], NULL, 10));
	


	int datalen = strlen("checkout");
	int temp = htonl(datalen);
	int n = write(sockfd, (char*)&temp, sizeof(temp));
	if (n < 0){
		fprintf(stderr, "Error. Cannot write to socket\n");
		exit(0);
	}
	n = write(sockfd, "checkout", datalen);
	if (n < 0){
		fprintf(stderr, "Error. Cannot write to socket\n");
		exit(0);
	}


	//for project name
	datalen = strlen(projectName);
	temp = htonl(datalen);
	n = write(sockfd, (char*)&temp, sizeof(temp));
	if (n < 0){
		fprintf(stderr, "Error. Cannot write to socket\n");
		exit(0);
	}
	n = write(sockfd, projectName , datalen);
	if (n < 0){
		fprintf(stderr, "Error. Cannot write to socket\n");
		exit(0);
	}




}


int main(int argc, char** argv) {
	char* ip;
	int port;



	//recvOne("blah", 0);


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
			checkout(argv[1]);
			printf("file exists\n");
		} else {
			fprintf(stderr, "Error. Configurations not set! No valid IP and port.\n");
			return 0;
		}
	} else if (strcmp("create", argv[1]) == 0) {
		if(access(".configure", F_OK) != -1) {
			create(argv[2]);
		}else {
			fprintf(stderr, "Error. Configurations not set! No valid IP and port.\n");
			return 0;
		}


	}
	return 0;



}
