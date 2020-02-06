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
#include <ctype.h>
#include "ftp.h"
#include "mtp.h"
#include "zlib.h"
#define BUFF 8192

struct node{
	char* version;
	char* addr;
	char* hash;
};

typedef struct list {
	pthread_t id;
	struct list *next;
} list;

list* head;

int run = 1;
void signalHandler() {
	run = 0;
	pthread_exit(0);
}

int childfd;


void handlerExit() {
	printf("\nTerminating...\n");
	pthread_exit(0);
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
	mkdir(".server_repo", 0700);

	//makes the file in ServerRepo
	char filePath[BUFF];
	bzero(filePath, BUFF);

	sprintf(filePath, ".server_repo/%s", projectName);
	//printf("attempting to create project in path: %s\n", filePath);

	if(mkdir(filePath, 0700) == 0){

		sprintf(filePath, "%s/.Manifest", filePath);
		pthread_mutex_lock(&mLock2);
		FILE *fd = fopen(filePath, "w");
		fprintf(fd, "0\n");
		fclose(fd);

		char historyFile[BUFF];
		bzero(historyFile, BUFF);
		sprintf(historyFile, ".server_repo/%s/.history", projectName, strlen(projectName));
		fd = fopen(historyFile, "w");
		fprintf(fd, "create\n0\n\n");
		fclose(fd);
		pthread_mutex_unlock(&mLock2);

		char* msg = encodeFile(filePath, "/.Manifest");
		sendMsg(msg, childfd);
		printf("message send\n");

	} else {
		//exit(0);
	}
}

int isFile(const char* path) {
	struct stat path_stat;
	stat(path, &path_stat);
	return S_ISREG(path_stat.st_mode);
}


void checkout(char* projectName, gzFile outFile, int childfd) {
	char path[BUFF];
	bzero(path, BUFF);
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
				bzero(buffer, BUFF);
				char fileLen[BUFF];
				bzero(fileLen, BUFF);
				char filePath[BUFF];
				bzero(filePath, BUFF);
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
					bzero(numStr, BUFF);
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
		//exit(0);
	}
	pthread_mutex_unlock(&mLock3);
}

//returns a string version of everything in the file
char* makeSentence(char* filePath){
	//if the file is being added for the second time
	int fd = open(filePath, O_RDONLY, 0644);
	if(fd == -1) {
		fprintf(stderr, "Error. Cannot find Manifest. \n");
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
	char* content= (char*) malloc(count * sizeof(char));
	fd = open(filePath, O_RDONLY, 0644);
	int i = 0;
	while(read(fd, &c, 1)) {
		content[i] = c;
		i++;
	}
	close(fd);

	return content;

}

void commit(char* projectName, int childfd) {
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


	//gets .Commit
	char commit[BUFF];
	bzero(commit, BUFF);
	recMsg(commit, childfd);

	//make the path and enter the contents into the file
	char* commitPath =(char*)malloc(strlen(projectName) + strlen("/.Commit") + strlen(".server_repo/"));
	memcpy(commitPath, ".server_repo/", strlen(".server_repo/"));
	strcat(commitPath, projectName);
	strcat(commitPath, "/.Commit");

	int fd = open(commitPath, O_CREAT | O_RDWR, 0644);
	write(fd, commit, strlen(commit));
	close(fd);

	pthread_mutex_unlock(&mLock8);


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
		bzero(buffer, BUFF);
		int x;

		while((x = fread(buffer, 1, sizeof(buffer), fp)) > 0) {
			sprintf(buffer, "%s", buffer);
		}

		sendMsg(buffer, childfd);
	} else {
		sendMsg("no path", childfd);
		//exit(0);
	}
	pthread_mutex_unlock(&mLock4);
}


void destroy(char* projectName) {
	pthread_mutex_lock(&mLock5);
	char projectPath[BUFF];
	bzero(projectPath, BUFF);
	sprintf(projectPath, ".server_repo/%s", projectName);

	if(folderExist(projectPath)) {
		sendMsg("folder exists", childfd);
		char rmBuff[BUFF];
		bzero(rmBuff, BUFF);
		sprintf(rmBuff, "rm -rf %s", projectPath, strlen(projectPath));
		system(rmBuff);
	} else {
		sendMsg("no path", childfd);
		//exit(0);
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
	bzero(snum, BUFF);
	snprintf(snum, sizeof(snum), "%d", sLines);
	sendMsg(snum, childfd);
	pthread_mutex_unlock(&mLock8);

	return;
	//exit(0);

}


void history(char* projectName) {
	pthread_mutex_lock(&mLock6);
	char path[BUFF];
	bzero(path, BUFF);
	sprintf(path, ".server_repo/%s", projectName);

	if(folderExist(path)) {
		sendMsg("folder exists", childfd);
		sprintf(path, "%s/.history", path, strlen(path), strlen("/.history"));

		FILE* fp = fopen(path, "r");
		char buffer[BUFF];
		bzero(buffer, BUFF);
		int x;

		while((x = fread(buffer, 1, sizeof(buffer), fp)) > 0) {
			sprintf(buffer, "%s", buffer);
		}

		sendMsg(buffer, childfd);
	} else {
		sendMsg("no path", childfd);
		//exit(0);
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
		//exit(0);
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

int wordCount(char* sentence){
	int count = 0;
	int flag = 0;
	int i;
	for(i = 0; i < strlen(sentence); i++){
		if(flag == 0 && !isspace(sentence[i])){
			count++;
			flag = 1;
		} else if (isspace(sentence[i])){
			flag = 0;
		}
	}
	return count;
}

char** makeArr(char* sentence, int numWords, char* delimiter){
	char** arr2 = (char**) malloc(numWords * sizeof(char*));
	char* word = strtok(sentence, delimiter);
	int count = 0;

	while(word != NULL){
		arr2[count] = word;
		word = strtok(NULL, delimiter);
		count++;
	}
	return arr2;
}



int digits_only(const char *s)
{
	while (*s) {
		if (isdigit(*s++) == 0) return 0;
	}

	return 1;
}


void push(char* projectName) {
	pthread_mutex_lock(&mLock9);
	char path[BUFF];
	sprintf(path, ".server_repo/%s", projectName);
	//printf("path: %s\n", path);

	//make sure everything exists before starting anything
	if(folderExist(path)) {
		sendMsg("folder exists", childfd);
		char clientCom[BUFF];
		bzero(clientCom, BUFF);
		recMsg(clientCom, childfd);

		char commitPath[BUFF];
		bzero(commitPath, BUFF);
		sprintf(commitPath, ".server_repo/%s/%s", projectName, "/.Commit", strlen(projectName), strlen("/.Commit"));


		FILE* fp = fopen(commitPath, "r");
		int x = 0;
		char serverCom[BUFF];
		bzero(serverCom, BUFF);
		while((x = fread(serverCom, 1, sizeof(serverCom), fp)) > 0) {
			sprintf(serverCom, "%s", serverCom);
		}
		//printf("server com: %s\n", serverCom);
		fclose(fp);

		//nothing in the commit
		char* commit = makeSentence(commitPath);
		// printf("%s\n", commit);
		int num = wordCount(commit);
		char** arr = makeArr(commit, num, "\t\n");

		//if not same --> return error
		if (strcmp(serverCom, clientCom) == 0) {

			sendMsg("success", childfd);

			//server manifest path
			char* manifestPath = (char*)malloc(strlen(projectName) + strlen("/.Manifest") + strlen(".server_repo/"));
			memcpy(manifestPath , ".server_repo/", strlen(".server_repo/"));
			strcat(manifestPath, projectName);
			strcat(manifestPath , "/.Manifest");

			//int sCom = getLines(commitPath);
			//printf("%d\n", sCom);

			//make an array out of the old manifest
			char* oldManifest = makeSentence(manifestPath);
			int maniLines = getLines(manifestPath) - 1;
			char* temp =  strtok(oldManifest, "\n");
			char* serverVersion = temp;
			struct node server[maniLines];
			int l = 0;
			while(l < maniLines){
				temp = strtok(NULL, "\t");
				server[l].version = temp;
				//printf("%s\n", server[l].version);

				temp = strtok(NULL, "\t");
				server[l].addr = temp;
				//printf("%s\n", server[l].addr);

				temp =  strtok(NULL, "\n");
				server[l].hash = temp;
				//printf("%s\n", server[l].hash);

				l++;
			}

			//makes path and writes into history to keep track for rollback 
			char* history = (char*)malloc(strlen(projectName) + strlen("/.Manifest") + strlen(".server_repo/"));
			memcpy(history , ".server_repo/", strlen(".server_repo/"));
			strcat(history, projectName);
			strcat(history , "/.History");

			int sd= open(history, O_APPEND | O_RDWR, 0644);
			write(sd, "push ", strlen("push "));
			write(sd, serverVersion, strlen(serverVersion));
			write(sd, commit, strlen(commit));

			close(sd);

			int i = 0 ;
			int upToDate[maniLines]; //everything is zero
			memset(upToDate, 0, maniLines);//everything is zero

			remove(manifestPath);

			int fd = open(manifestPath, O_CREAT | O_RDWR, 0644);
			int serverLife = atoi(serverVersion);
			serverLife++;
			snprintf(serverVersion, strlen(serverVersion), "%d", serverLife);
			write(fd, serverVersion, strlen(serverVersion));
			write(fd, "\n", strlen("\n"));
			//close(fd);

			for(i = 0; i < num; i++){
				char* letter = arr[i];
				//printf("%s\n", letter);
				i++;
				char* version = arr[i];
				//printf("%s\n", version);
				i++;
				char* path = arr[i];
				//printf("%s\n", path);
				i++;
				char* hash = arr[i];
				//printf("%s\n", hash);

				int j = 0;
				//printf("%s\n", "done!");
				//iterate through the manifest
				for(j = 0; j < maniLines; j++){
					if(strcmp(letter, "D") == 0){
						remove(path);
						upToDate[j] = 1;
					} else if(strcmp(letter, "A") == 0){
						upToDate[j] = 1;
						// printf("A: %s\n", path);
						//add to the manifest
						write(fd, version, strlen(version));
						write(fd, "\t", strlen("\t"));
						write(fd, path, strlen(path));
						write(fd, "\t", strlen("\t"));
						write(fd, hash, strlen(hash));
						write(fd, "\n", strlen("\n"));


						//make path in server
						char* filePath = (char*)malloc(strlen(".server_repo/") + strlen(path));
						memcpy(filePath, ".server_repo/", strlen(".server_repo/"));
						strcat(filePath, path);

					} else if(strcmp(letter, "U") == 0){
						upToDate[j] = 1;
						//add to the manifest

						// printf("U: %s\n", path);
						write(fd, version, strlen(version));
						write(fd, "\t", strlen("\t"));
						write(fd, path, strlen(path));
						write(fd, "\t", strlen("\t"));
						write(fd, hash, strlen(hash));
						write(fd, "\n", strlen("\n"));

						//make path in server
						char* filePath = (char*)malloc(strlen(".server_repo/") + strlen(path));
						memcpy(filePath, ".server_repo/", strlen(".server_repo/"));
						strcat(filePath, path);

					}

				}
			}

			//printf("%s\n", "done!");

			//add the files that were upto date in the orginal manifest
			for(i = 0; i<maniLines; i++){
				if(upToDate[i] == 0){
					write(fd, server[i].version, strlen(server[i].version));
					write(fd, "\t", strlen("\t"));
					write(fd, server[i].addr, strlen(server[i].addr));
					write(fd, "\t", strlen("\t"));
					write(fd, server[i].hash, strlen(server[i].hash));
					write(fd, "\n", strlen("\n"));

				}
			}

			//printf("%s\n", "done!");

			close(fd);
			remove(commitPath);
			return;
		} else {
			sendMsg("fail", childfd);
			return;
		}

	} else {
		sendMsg("no path", childfd);
		//exit(0);
	}
	pthread_mutex_unlock(&mLock9);

	return;

}

void rollback(char* projectName) {
	pthread_mutex_lock(&mLock10);
	char projectPath[BUFF];
	bzero(projectPath, BUFF);
	sprintf(projectPath, ".server_repo/%s", projectName);

	if(folderExist(projectPath)) {
		sendMsg("folder exists", childfd);
		char ver[BUFF];
		bzero(ver, BUFF);
		recMsg(ver, childfd);

		char histPath[BUFF];
		bzero(histPath, BUFF);
		sprintf(histPath, "%s%s", projectPath, "/.history");

		FILE* fp = fopen(histPath, "r");
		int x = 0;
		char histContent[BUFF];
		bzero(histContent, BUFF);

		while((x = fread(histContent, 1, sizeof(histContent), fp)) > 0) {
			sprintf(histContent, "%s", histContent);
		}
		fclose(fp);

		//printf("HIST CONTENT\n=========================\n%s\n", histContent);

		char** currLineArr;
		char* curLine = histContent;
		int flag = 0;
		int iVer;
		sscanf(ver, "%d", &iVer);

		char rbMan[BUFF];
		bzero(rbMan, BUFF);

		char rollback[BUFF];
		bzero(rollback, BUFF);

		int done = 0;
		int onlyOnce = 0;
		int currVer;

		if(iVer == 0) {
			sprintf(rollback, "rollback 0\n0\n", strlen("rollback 0\n0\n"));
		} else {
			sprintf(rollback, "%s%s%d\n", rollback, "rollback ", iVer, strlen(rollback), strlen("rollback "));
			while(curLine) {
				char * nextLine = strchr(curLine, '\n');
				if (nextLine) *nextLine = '\0';  // temporarily terminate the current line
				int count = wordCount(curLine);
				currLineArr = makeArr(curLine, count, "\t");

				if(count == 1 && digits_only(currLineArr[0]) == 1) {
					sscanf(currLineArr[0], "%d", &currVer);
					if(iVer == currVer) {
						if(onlyOnce > 1) {
							break;
						}
						flag = 1;
						onlyOnce++;
					} else {
						flag = 0;
					}
				} else if (count == 1 && digits_only(currLineArr[0]) != 1){
					flag = 0;
				}

				if (flag == 1){
					if(count == 1 && digits_only(currLineArr[0]) == 1) {

						sprintf(rollback, "%s%s\n", rollback, currLineArr[0], strlen(rollback),strlen(currLineArr[0]));
						sprintf(rbMan, "%s%s\n", rbMan, currLineArr[0], strlen(rbMan),strlen(currLineArr[0]));
					} else if (count == 4) {
						sprintf(rollback, "%s%s\t%s\t%s\t%s\n", rollback, currLineArr[0], currLineArr[1], currLineArr[2], currLineArr[3], strlen(rollback), strlen(currLineArr[0]), strlen(currLineArr[1]), strlen(currLineArr[2]), strlen(currLineArr[3]));
						sprintf(rbMan, "%s%s\t%s\t%s\n", rbMan, currLineArr[1], currLineArr[2], currLineArr[3], strlen(rbMan), strlen(currLineArr[1]), strlen(currLineArr[2]), strlen(currLineArr[3]));
					}
				}
				if (nextLine) *nextLine = '\n';
				curLine = nextLine ? (nextLine+1) : NULL;
			}
		}

		if(strlen(rbMan) > 0) {
			char manPath[BUFF];
			bzero(manPath, BUFF);
			sprintf(manPath, "%s%s",projectPath, "/.Manifest");
			remove(manPath);

			char rb[BUFF];
			bzero(rb, BUFF);
			fp = fopen(manPath, "w");
			fprintf(fp, "%s", rbMan);
			fclose(fp);

			FILE* pFile2 = fopen(histPath, "a");
			fprintf(pFile2, "\n%s", rollback);
			fclose(pFile2);

			bzero(rbMan, BUFF);
			bzero(rollback, BUFF);

			sendMsg("success", childfd);
		} else {
			sendMsg("invalid version", childfd);
		}



	} else {
		sendMsg("no path", childfd);
	}






	pthread_mutex_unlock(&mLock10);
}




void* getCommand() {
	//pthread_mutex_lock(&mLock1);
	char command[BUFF];
	bzero(command, BUFF);
	recMsg(command, childfd);
	if(strcmp("create", command) == 0) {
		char projectName[BUFF];
		recMsg(projectName, childfd);

		create(projectName);
	} else if(strcmp("checkout", command) == 0) {
		char projectName[BUFF];
		bzero(projectName, BUFF);
		char gzLoc[BUFF];
		bzero(gzLoc, BUFF);

		recMsg(projectName, childfd);
		sprintf(gzLoc, ".server_repo/%s/zippedFile.gz", projectName);

		gzFile outFile = gzopen(gzLoc, "wb");
		checkout(projectName, outFile, childfd);
		gzclose(outFile);

		char msg[BUFF];
		bzero(msg, BUFF);
		gzFile f = gzopen(gzLoc, "r");
		sendCompress(gzLoc, childfd);
		write(childfd, f, BUFF);
		remove(gzLoc);
		gzclose(f);
	} else if (strcmp("commit", command) == 0) {
		char projectName[BUFF];
		bzero(projectName, BUFF);
		recMsg(projectName, childfd);
		commit(projectName, childfd);
	} else if (strcmp("currentversion", command) == 0) {
		char projectName[BUFF];
		bzero(projectName, BUFF);
		recMsg(projectName, childfd);
		currentVersion(projectName, childfd);
	} else if (strcmp("destroy", command) == 0) {
		char projectName[BUFF];
		bzero(projectName, BUFF);
		recMsg(projectName, childfd);
		destroy(projectName);
	} else if(strcmp("update", command) == 0){
		char projectName[BUFF];
		bzero(projectName, BUFF);
		recMsg(projectName, childfd);

		update(projectName, childfd);
	} else if(strcmp("history", command) == 0) {
		char projectName[BUFF];
		bzero(projectName, BUFF);
		recMsg(projectName, childfd);
		history(projectName);
	} else if(strcmp("upgrade", command) == 0) {
		char projectName[BUFF];
		bzero(projectName, BUFF);
		recMsg(projectName, childfd);
		upgrade(projectName);
	} else if(strcmp("push", command) == 0) {
		char projectName[BUFF];
		bzero(projectName, BUFF);
		recMsg(projectName, childfd);
		push(projectName);
	} else if(strcmp("rollback", command) == 0) {
		char projectName[BUFF];
		bzero(projectName, BUFF);
		recMsg(projectName, childfd);
		rollback(projectName);
	} else {
		printf("invalid command: %s\n", command);
	}

	//pthread_mutex_unlock(&mLock1);
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

	list* temp = (list*) malloc(sizeof(struct list));
	head = temp;
	head -> next = NULL;

	void* vport = &port;

	while(1) {

		connfd = accept(sockfd, (struct sockaddr*)&client, &len);
		if(connfd < 0) {
			fprintf(stderr, "Error. Server accept failed.\n");
			//exit(0);
		} else {
			printf("Server accepted to client!\n");
		}

		childfd = connfd;
		pthread_create(&head->id, NULL, &commandThread, vport);
		pthread_join(head->id, NULL);
		//list* t1 = head;
		//list* oldThread = head;
		//t1 = t1->next;
	}

	pthread_exit(0);


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
	//list* t1 = head;
	//list* oldThread = head;
	//t1 = t1->next;
	//free(oldThread);

	close(childfd);


	return 0;
}
