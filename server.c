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
#include <semaphore.h>
#include "ftp.h"
#include "mtp.h"
#include "zlib.h"
#define BUFF 8192

typedef struct list {
	pthread_t id;
	struct list *next;
} list;

list* head;

int run = 1;
void signalHandler() {
	run = 0;
}

int childfd;


void handlerExit() {
	printf("\nTerminating...\n");
}


pthread_mutex_t mLock1 = PTHREAD_MUTEX_INITIALIZER;
pthread_mutex_t mLock2 = PTHREAD_MUTEX_INITIALIZER;
pthread_mutex_t mLock3 = PTHREAD_MUTEX_INITIALIZER;
pthread_mutex_t mLock4 = PTHREAD_MUTEX_INITIALIZER;
pthread_mutex_t mLock5 = PTHREAD_MUTEX_INITIALIZER;
pthread_mutex_t mLock6 = PTHREAD_MUTEX_INITIALIZER;
pthread_mutex_t mLock7 = PTHREAD_MUTEX_INITIALIZER;
pthread_mutex_t mLock8 = PTHREAD_MUTEX_INITIALIZER;
pthread_mutex_t mLock9 = PTHREAD_MUTEX_INITIALIZER;
pthread_mutex_t mLock10 = PTHREAD_MUTEX_INITIALIZER;


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

void create(char* projectName){
	pthread_mutex_lock(&mLock2);
	mkdir(".server_repo", 0700);

	//makes the file in ServerRepo
	char filePath[BUFF];

	sprintf(filePath, ".server_repo/%s", projectName);
	//printf("attempting to create project in path: %s\n", filePath);

	if(mkdir(filePath, 0700) == 0){

		sprintf(filePath, "%s/.Manifest", filePath);
		FILE *fd = fopen(filePath, "w");
		fprintf(fd, "1\n");
		fclose(fd);

		char historyFile[BUFF];
		sprintf(historyFile, ".server_repo/%s/.history", projectName, strlen(projectName));
		fd = fopen(historyFile, "w");
		fprintf(fd, "create\n0\n\n");
		fclose(fd);

		char* msg = encodeFile(filePath, "/.Manifest");
		sendMsg(msg, childfd);

	} else {
		exit(0);
	}
	pthread_mutex_unlock(&mLock2);
}

int isFile(const char* path) {
	struct stat path_stat;
	stat(path, &path_stat);
	return S_ISREG(path_stat.st_mode);
}


void checkout(char* projectName, gzFile outFile, int childfd) {
	char path[BUFF];
	sprintf(path, ".server_repo/%s", projectName);
	pthread_mutex_lock(&mLock3);
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
	pthread_mutex_unlock(&mLock3);
}

void commit(char* projectName, int childfd) {
	printf("commit\n");
	char path[BUFF];
	bzero(path, BUFF);
	sprintf(path, ".server_repo/%s", projectName);

	if(folderExist(path)) {
		sendMsg("folder exists", childfd);
	} else {
		sendMsg("no path", childfd);
		exit(0);
	}


}

void currentVersion(char* projectName, int childfd) {
	pthread_mutex_lock(&mLock4);
	char projectPath[BUFF];
	bzero(projectPath, BUFF);
	sprintf(projectPath, ".server_repo/%s", projectName);

	if(folderExist(projectPath)) {
		sendMsg("folder exists", childfd);
		sprintf(projectPath, "%s/.Manifest", projectPath, strlen(projectPath), strlen("/.Manifest"));

		FILE* fp = fopen(projectPath, "r");
		char buffer[BUFF];
		int x;

		while((x = fread(buffer, 1, sizeof(buffer), fp)) > 0) {
			sprintf(buffer, "%s", buffer);
		}

		sendMsg(buffer, childfd);
	} else {
		sendMsg("no path", childfd);
		exit(0);
	}
	pthread_mutex_unlock(&mLock4);
}


void destroy(char* projectName) {
	pthread_mutex_lock(&mLock5);
	printf("destroy\n");
	char projectPath[BUFF];
	bzero(projectPath, BUFF);
	sprintf(projectPath, ".server_repo/%s", projectName);
	printf("%s\n", projectPath);

	if(folderExist(projectPath)) {
		sendMsg("folder exists", childfd);
		char rmBuff[BUFF];
		sprintf(rmBuff, "rm -rf %s", projectPath, strlen(projectPath));
		printf("%s\n", rmBuff);
		system(rmBuff);
	} else {
		sendMsg("no path", childfd);
		exit(0);
	}

	pthread_mutex_unlock(&mLock5);


}

//returns the number of lines in a file
int getLines(char* fileName){
	int i = 0;
	FILE *fp;

	fp = fopen(fileName, "r");

	if(fp == NULL){
		fprintf(stderr, "Error. Cannot find file(s)\n");
	}

	char c = getc(fp);

	while((c = getc(fp)) != EOF){
		if(c == '\n'){
			++i;
		}
	}

	fclose(fp);
	return i;

}

void update(char* projectName, int childfd){

	pthread_mutex_lock(&mLock8);

	//get the project path
	char* projectPath = (char*)malloc(strlen(projectName)+ strlen(".server_repo/"));
	memcpy(projectPath, ".server_repo/", strlen(".server_repo/"));
	strcat(projectPath, projectName);

	//checks if the folder exists
	DIR *de;
	if ((de = opendir(projectPath)) == NULL){
		sendMsg("not found", childfd);
	} else {
		sendMsg("found", childfd);
	}

	//get the server manifest path
	char* manifestPath = (char*)malloc(strlen(projectPath)+ strlen("/.Manifest"));
	memcpy(manifestPath, projectPath, strlen(projectPath));
	strcat(manifestPath, "/.Manifest");

	//gets the encoded message and sends it to the client
	char* msg = encodeFile(manifestPath, ".Manifest");
	sendMsg(msg, childfd);

	//send file size
	int sLines = getLines(manifestPath);
	char snum[BUFF];
	snprintf(snum, sizeof(snum), "%d", sLines);
	sendMsg(snum, childfd);
	pthread_mutex_unlock(&mLock8);

	return;
	exit(0);

}


void history(char* projectName) {
	pthread_mutex_lock(&mLock6);
	char path[BUFF];
	sprintf(path, ".server_repo/%s", projectName);

	if(folderExist(path)) {
		sendMsg("folder exists", childfd);
		sprintf(path, "%s/.history", path, strlen(path), strlen("/.history"));

		FILE* fp = fopen(path, "r");
		char buffer[BUFF];
		int x;

		while((x = fread(buffer, 1, sizeof(buffer), fp)) > 0) {
			sprintf(buffer, "%s", buffer);
		}

		sendMsg(buffer, childfd);
	} else {
		sendMsg("no path", childfd);
		exit(0);
	}
	pthread_mutex_unlock(&mLock6);
}

void upgrade(char* projectName) {
	pthread_mutex_lock(&mLock7);

	char path[BUFF];
	sprintf(path, ".server_repo/%s", projectName);

	if(folderExist(path)) {
		sendMsg("folder exists", childfd);
		
	} else {
		sendMsg("no path", childfd);
		exit(0);
	}

	int count = 0;
	char msg[BUFF];
	while(1) {
		bzero(msg, BUFF);
		recMsg(msg, childfd);
		if(strcmp("NickAndVancha'sUniqueKeyThatShouldn'tBeAFileToTerminateAndFinish", msg) == 0) {
			break;
		} else {
			char filePath[BUFF];
			sprintf(filePath, ".server_repo/%s", msg, strlen(msg));

			char* fileName = filePath;
			fileName += strlen(".server_repo/");

			char* encode = encodeFile(filePath, fileName);
			sendMsg(encode, childfd);
		}
	}
	pthread_mutex_unlock(&mLock7);
}




void* getCommand() {
	pthread_mutex_lock(&mLock1);
	char command[BUFF];
	recMsg(command, childfd);
	if(strcmp("create", command) == 0) {
		char projectName[BUFF];
		recMsg(projectName, childfd);

		create(projectName);
	} else if(strcmp("checkout", command) == 0) {
		char projectName[BUFF];
		char gzLoc[BUFF];

		recMsg(projectName, childfd);
		sprintf(gzLoc, ".server_repo/%s/zippedFile.gz", projectName);

		gzFile outFile = gzopen(gzLoc, "wb");
		checkout(projectName, outFile, childfd);
		gzclose(outFile);

		char msg[BUFF];
		gzFile f = gzopen(gzLoc, "r");
		sendCompress(gzLoc, childfd);
		write(childfd, f, BUFF);
		remove(gzLoc);
		gzclose(f);
	} else if (strcmp("commit", command) == 0) {
		char projectName[BUFF];
		recMsg(projectName, childfd);
		commit(projectName, childfd);
	} else if (strcmp("currentversion", command) == 0) {
		char projectName[BUFF];
		recMsg(projectName, childfd);
		currentVersion(projectName, childfd);
	} else if (strcmp("destroy", command) == 0) {
		char projectName[BUFF];
		recMsg(projectName, childfd);
		destroy(projectName);
	} else if(strcmp("update", command) == 0){
		char projectName[BUFF];
		recMsg(projectName, childfd);

		update(projectName, childfd);
	} else if(strcmp("history", command) == 0) {
		char projectName[BUFF];
		recMsg(projectName, childfd);
		history(projectName);
	} else if(strcmp("upgrade", command) == 0) {
		char projectName[BUFF];
		recMsg(projectName, childfd);
		upgrade(projectName);
	}

	pthread_mutex_unlock(&mLock1);
}

void* commandThread(void* socket) {
	getCommand(*(int*) socket);
}


void* createServer(int port) {
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

	childfd = connfd;



	list* temp = (list*) malloc(sizeof(struct list));
	head = temp;
	head -> next = NULL;

	void* vport = &port;

	pthread_create(&head->id, NULL, &commandThread, vport);
	pthread_join(head->id, NULL);
	list* t1 = head;
	list* oldThread = head;
	t1 = t1->next;

}

void* myThread(void* port) {
	createServer(*(int*)port);
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

	list* temp = (list*) malloc(sizeof(struct list));
	head = temp;
	head -> next = NULL;

	void* vport = &port;

	pthread_create(&head->id, NULL, myThread, vport);
	pthread_join(head->id, NULL);
	list* t1 = head;
	list* oldThread = head;
	t1 = t1->next;
	free(oldThread);

	getCommand(childfd);


	close(childfd);
	exit(0);

	return 0;
}
