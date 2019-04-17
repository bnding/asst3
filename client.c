#include <stdlib.h>
#include <stdio.h>
#include <signal.h>
#include <sys/types.h>
#include <string.h>
#include <sys/socket.h>
#include <netdb.h>
#include <dirent.h>
#include <fcntl.h>

#define BUFSIZE 1024



//connects client and server
int connectServer(char* cmd, char* ip, int port) {
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
	if(intPort < 8000 || intPort > 64000) {
		fprintf(stderr, "Invalid port. Need to be >8k and <64k.\nTerminating.\n");
		exit(0);
	}
	int fd;

	fd = open(".configure", O_CREAT | O_RDWR, 0644);
	write(fd, ip, strlen(ip));
	write(fd, " ", 1);
	write(fd, port, strlen(port));


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
		configStr[i] += c;
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

	return config2d;


}


void create(char* projectName) {
	int n;

	char** config = readConfig();
	int sockfd = connectServer("create", config[0], strtol(config[1], NULL, 10));


	//send the message line to the server
	n = write(sockfd, projectName, strlen(projectName));
	if (n < 0){
		fprintf(stderr, "Error. Cannot write to socket");
		exit(0);
	}

	close(sockfd);
}


int main(int argc, char** argv) {
	char* ip;
	int port;
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
