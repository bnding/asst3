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
#include <fcntl.h>
#include "ftp.h"
#include "mtp.h"
#include "zlib.h"

#define BUFF 8192


int run = 1;

void *myThread(void *vargp) {
	int i = 0;
	while(1) {
		printf("thread workload: %d\n", i);
		i++;
		sleep(1);
	}
	return NULL;
}

void signalHandler() {
	run = 0;
}

void handlerExit() {
	printf("\nTerminating...\n");
}

int createServer(int port) {
	int connfd, len;
	struct sockaddr_in serverAddr, client;

	int sockfd = socket(AF_INET, SOCK_STREAM, 0);
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
		return 1;
	}
	return 0;

}

void create(char* projectName, int childfd){
	printf("project name: %s\n", projectName);
	printf("length: %d\n", strlen(projectName));
	//creates ServerRepo if dne
	if(folderExist(".server_repo") == 0){
		mkdir(".server_repo", 0700);
	}

	//makes the file in ServerRepo 
	char filePath[BUFF];

	sprintf(filePath, ".server_repo/%s", projectName);
	printf("attempting to create project in path: %s\n", filePath);

	if(folderExist(filePath) == 0){
		mkdir(filePath, 0700);

		sprintf(filePath, "%s/.Manifest", filePath);
		int fd = open(filePath, O_CREAT | O_RDWR, 0644);
		close(fd);

		char* msg = encodeFile(filePath, "/.Manifest");
		sendMsg(msg, childfd);

	} else {
		exit(0);
	}
}

int isFile(const char* path) {
	struct stat path_stat;
	stat(path, &path_stat);
	return S_ISREG(path_stat.st_mode);
}


void checkout(char* projectName, gzFile outFile, int childfd) {
	char path[BUFF];
	sprintf(path, ".server_repo/%s", projectName);
	if (folderExist(path)) {
		sendMsg("exists", childfd);
		DIR *dir = opendir(path);
		struct dirent* entry;

		if(!(dir = opendir(path))) {
			return;
		}

		FILE *fp;
		char ch;
		int size = 0;

		unsigned long totalRead = 0;
		unsigned long totalWrite = 0;

		while ((entry = readdir(dir)) != NULL) {

			if (strcmp(entry->d_name, ".") != 0 && strcmp(entry->d_name, "..") != 0 && strcmp(entry->d_name, "zippedFile.gz") != 0){
				int x = 0;
				char buffer[BUFF];
				char fileLen[BUFF];
				char filePath[BUFF];
				sprintf(filePath, "%s/%s", path, entry->d_name);
				gzwrite(outFile, filePath, strlen(filePath));

				if (entry->d_type == DT_DIR) {
					//directory
					char path[1024];
					snprintf(path, sizeof(path), "%s/%s", projectName, entry->d_name);
					gzwrite(outFile, " D -1\n", strlen(" D -1\n"));
					checkout(path, outFile, childfd);
				} else {
					//files
					gzwrite(outFile, " R ", strlen(" R "));
					fp = fopen(filePath, "rb");
					while((x = fread(fileLen, 1, sizeof(fileLen), fp)) > 0) {
					}
					fseek(fp, 0L, SEEK_END);
					size = ftell(fp);
					char numStr[BUFF];
					snprintf(numStr, sizeof(numStr), "%d", size);
					gzwrite(outFile, numStr, strlen(numStr));
					fclose(fp);

					x = 0;
					fp = fopen(filePath, "rb");
					gzwrite(outFile, "\n", strlen("\n"));
					while((x = fread(buffer, 1, sizeof(buffer), fp)) > 0) {
						totalRead += x;
						sprintf(buffer, "%s", buffer);
						gzwrite(outFile, buffer, x);
					}	       
					fseek(fp, 0L, SEEK_END);
					size = ftell(fp);
					fclose(fp);
				}
			}
		}
		closedir(dir);
	} else {
		sendMsg("no path", childfd);
		exit(0);
	}
}



void getCommand(int childfd) {
	char command[BUFF];
	bzero(command, BUFF);

	recMsg(command, childfd);
	printf("command: %s\n\n", command);

	if(strcmp("create", command) == 0) {
		char projectName[BUFF];

		recMsg(projectName, childfd);

		create(projectName, childfd);
	} else if(strcmp("checkout", command) == 0) {
		char projectName[BUFF];
		char gzLoc[BUFF];

		recMsg(projectName, childfd);
		sprintf(gzLoc, ".server_repo/%s/zippedFile.gz", projectName);

		gzFile outFile = gzopen(gzLoc, "wb");
		checkout(projectName, outFile, childfd);
		gzclose(outFile);

		char msg[BUFF];
		FILE *f = fopen(gzLoc, "r");
		fscanf(f, "%s", msg);
		printf("message sent\n======================\n%s", msg);
		write(childfd, msg, BUFF);
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
	int childfd = createServer(port);

	getCommand(childfd);




	// pthread_t thread_id;
	// pthread_create(&thread_id, NULL, myThread, NULL);
	// pthread_join(thread_id, NULL);


	//close(sockfd);
	close(childfd);
	exit(0);

	return 0;
}
