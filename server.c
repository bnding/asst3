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

#define BUFSIZE 1024


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

void serverTalk(int sockfd, int childfd){

	char* projectName[BUFSIZE];
	int n;
	struct dirent *dd;

	//read input string from the client
	bzero(projectName, BUFSIZE);
	n = read(childfd, projectName, BUFSIZE);
	if (n < 0){
		fprintf(stderr, "Error, Cannot read from socket");
	}

	printf("Server received: %s \n", projectName);

	//opens directory and checks if server repo exists
    	DIR *dr = opendir(".");

    	if (dr == NULL){
        	fprintf(stderr, "Error. Cannot open current directory" );
		return;
    	}

	//change asst2 to be repository folder name
	//i just made it asst2 so that i could test that i would stop
	int i = 0;
    	while ((dd = readdir(dr)) != NULL){
		if((dd->d_type == DT_DIR) && (strcmp(dd->d_name, "asst2") == 0)){
			printf("found %s\n", dd->d_name);
			i = 1;
			break;
		}
	}

	//makes the repository if it dne
	if(i == 0){
		//make the repository
	}

	//add the projectName to repository

    	closedir(dr);

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

	//printf("here\n");

	serverTalk(sockfd, childfd);




	// pthread_t thread_id;
	// pthread_create(&thread_id, NULL, myThread, NULL);
	// pthread_join(thread_id, NULL);


	close(sockfd);
	close(childfd);
	exit(0);

	return 0;
}
