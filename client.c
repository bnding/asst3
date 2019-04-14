#include <stdlib.h>
#include <stdio.h>
#include <signal.h>
#include <sys/types.h>
#include <string.h>
#include <sys/socket.h>
#include <netdb.h>

char* getCommands(char* cmd) {
	if(strcmp("configure", cmd) == 0) {
		return "configure";
	} else {
		fprintf(stderr, "Error. Invalid command!\n");
		return NULL;
	}
}

void configure(int argc, char* ip, int port) {
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

int main(int argc, char** argv) {
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
		configure(argc, argv[2], strtol(argv[3], NULL, 10));
	}


	return 0;



}
