#include <stdlib.h>
#include <stdio.h>
#include <unistd.h>
#include <pthread.h>
#include <signal.h>
#include <netdb.h>
#include <string.h>
#include <sys/socket.h>
#include <sys/types.h>
#include <dirent.h>
#include <sys/stat.h>

#define BUFF 1024


static int keepRunning = 1;

void *myThread(void *vargp) {
	int i = 0;
	while(keepRunning) {
		printf("thread workload: %d\n", i);
		i++;
		sleep(1);
	}
	return NULL;
}

void signalHandler() {
	keepRunning = 0;
}

void handlerExit() {
	printf("\nTerminating...\n");
}

int createServer(int port, int sockfd) {
	int connfd, len;
	struct sockaddr_in serverAddr, client;

	sockfd = socket(AF_INET, SOCK_STREAM, 0);
	if(sockfd < 0) {
		fprintf(stderr, "Error. Socket creation failed.\n");
		exit(0);
	} else {
		printf("Socket successfully created!\n");
	}

	bzero(&serverAddr, sizeof(serverAddr));

	if(port < 8000 || port > 64000) {
		fprintf(stderr, "Error. Server port is not within range!\n");
		exit(0);
	}

	serverAddr.sin_family = AF_INET;
	serverAddr.sin_addr.s_addr = htonl(INADDR_ANY);
	serverAddr.sin_port = htons(port);

	if((bind(sockfd, (struct sockaddr*)&serverAddr, sizeof(serverAddr))) != 0) {
		fprintf(stderr, "Error. socket bind failed.\n");
		exit(0);
	} else {
		printf("Socket successfully binded!\n");
	}

	if ((listen(sockfd, 5)) != 0) {
		fprintf(stderr, "Error. Listen failed.\n");
		exit(0);
	} else {
		printf("Server listening...\n");
	}
	len = sizeof(client);

	connfd = accept(sockfd, (struct sockaddr*)&client, &len);
	if(connfd < 0) {
		fprintf(stderr, "Error. Server accept failed.\n");
		exit(0);
	} else {
		printf("Server accepted to client!\n");
	}

	return connfd;

}

//takes in the name of the folder its looking for
//returns 1 if folder exists
int folderExist(char* lookingFor){
	struct stat buffer;
	stat(lookingFor, &buffer);

    	if (S_ISDIR(buffer.st_mode)){
		printf("Is a dir %s\n", lookingFor);
	    	return 1;
    	}
	printf("Is not a dir %s\n", lookingFor);

	return 0;

}

void serverCreate(int childfd){

	char projectName[BUFF];
	int n;

	//read input string from the client
	bzero(projectName, BUFF);
	n = read(childfd, projectName, BUFF);
	if (n < 0){
		fprintf(stderr, "Error, Cannot read from socket");
		exit(0);
	}

	//creates serverRepo if dne
	if(folderExist("serverRepo") == 0){
		mkdir("serverRepo", 0700);
	}

	//makes the file in serverRepo
	char filePath[BUFF];


	// memcpy(filepath, "serverRepo/", strlen( "serverRepo/%s"));
 	// memcpy(filePath, "serverRepo/", strlen("serverRepo/"));
	// filePath[strlen("serverRepo/")] = '\0';
	// strcat(filePath, projectName);

	sprintf(filePath, "serverRepo/%s", projectName);
	// filePath[strlen( "serverRepo/") + strlen(projectName)] = '\0';
	printf("%s\n", filePath);

	if(folderExist(filePath) == 0){
		mkdir(filePath, 0700);
	} else {
		fprintf(stderr, "Error, project with that name already exits\n");
	}


}


int main(int argc, char** argv) {
	if(argc < 2) {
		fprintf(stderr, "Error. Invalid port number.\n");
		return 0;
	}


	atexit(handlerExit);
	signal(SIGINT, signalHandler);

	int server = socket(AF_INET, SOCK_STREAM, 0);
	if(server < 0) {
		fprintf(stderr, "Error creating server!\n");
		exit(0);
	} else {
		printf("Server creation is successful!\n");
	}

	int port = strtol(argv[1], NULL, 10);
	int sockfd;

	int childfd = createServer(port, sockfd);

	serverCreate(childfd);




	// pthread_t thread_id;
	// pthread_create(&thread_id, NULL, myThread, NULL);
	// pthread_join(thread_id, NULL);


	close(sockfd);
	close(childfd);
	exit(0);

	return 0;
}
