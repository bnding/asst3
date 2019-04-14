#include <stdlib.h>
#include <stdio.h>
#include <signal.h>
#include <sys/types.h>
#include <string.h>
#include <sys/socket.h>
#include <netdb.h>
#include <dirent.h>
#include <fcntl.h>

char* getCommands(char* cmd) {
	if(strcmp("configure", cmd) == 0) {
		return "configure";
	} else {
		fprintf(stderr, "Error. Invalid command!\n");
		return NULL;
	}
}

void connectServer(int argc, char* ip, int port) {
	printf("IP: %s\n", ip);
	printf("port: %d\n", port);

	int sock = 0; 
	struct sockaddr_in serverAddr; 

	sock = socket(AF_INET, SOCK_STREAM, 0);
	printf("%d\n\n", sock);

	if(sock < 0) {
		fprintf(stderr, "Error. Socket creation failed.\n");
		exit(0);
	} else {
		printf("Socket creation is successful!\n");
	}

	memset(&serverAddr, '0', sizeof(serverAddr));
	//bzero(&serverAddr, sizeof(serverAddr));

	serverAddr.sin_family = AF_INET;
	serverAddr.sin_addr.s_addr = inet_addr(ip);
	serverAddr.sin_port = htons(port);


	if(connect(sock, (struct sockaddr*) &serverAddr, sizeof(serverAddr)) < 0) {
		fprintf(stderr, "Error. Connection to server failed.\n");
		exit(0);
	} else {
		printf("Successfully connected to server!\n");
	}
}

void configure(int argc, char* ip, char* port) {
	int fd;
	char c;
	int bytesread;
	int count = 0;

	fd = open(".configure", O_CREAT | O_RDWR, 0644);
	write(fd, ip, strlen(ip));
	write(fd, " ", 1);
	write(fd, port, strlen(port));


}

int main(int argc, char** argv) {
	//note : use strtol(argv[3], NULL, 10) to convert port number from string to int


	char* cmd;
	if(argc < 3) {
		fprintf(stderr, "Error. Invalid number of inputs.\n");
		return 0;
	} else {
		cmd = (char*) malloc(strlen(argv[1]) * sizeof(char));
		cmd = getCommands(argv[1]);
	}	

	if(strcmp("configure", argv[1]) == 0) {
		if(argc < 4) {
			fprintf(stderr, "Error. Invalid number of inputs for configure.\n");
			return 0;
		}
		configure(argc, argv[2], argv[3]);
		printf("Configured!\nIP: %s\nPort: %s\n", argv[2], argv[3]);
	}


	return 0;



}
