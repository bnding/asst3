#include <stdlib.h>
#include <stdio.h>
#include <unistd.h>
#include <pthread.h>
#include <signal.h>
#include <netdb.h>


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
	printf("\nSIGINT caught! Terminating...\n");
}

int main(int argc, char** argv) {
	if(argc < 2) {
		fprintf(stderr, "Error. Invalid port number.\n");
		return 0;
	}

	struct sockaddr_in serverAddr;

	atexit(handlerExit);
	signal(SIGINT, signalHandler);

	int server = socket(AF_INET, SOCK_STREAM, 0);
	if(server < 0) {
		fprintf(stderr, "Error creating server!\n");
		return 0;
	} else {
		printf("Server creation is successful!\n");
	}

	int port = strtol(argv[1], NULL, 10);
	bzero((char*) &serverAddr, sizeof(serverAddr));
	serverAddr.sin_family = AF_INET;
	serverAddr.sin_addr.s_addr = htonl(INADDR_ANY);
	serverAddr.sin_port = htons(port);



	pthread_t thread_id;
	pthread_create(&thread_id, NULL, myThread, NULL);
	pthread_join(thread_id, NULL);
	exit(0);

	return 0;
}
