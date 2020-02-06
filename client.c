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
#include "mtp.h"
#include "zlib.h"
#define BUFF 8192

struct node{
	char* version;
	char* addr;
	char* hash;
};

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
	//printf("token: %s\n", token);
	char** config2d = (char**) malloc(2 * sizeof(char*));

	config2d[0] = (char*) malloc(strlen(token) * sizeof(char));
	config2d[0] = token;

	token = strtok(NULL, " ");
	//printf("token: %s\n", token);
	config2d[1] = (char*) malloc(strlen(token) * sizeof(char));
	config2d[1] = token;

	return config2d;
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

//returns the number of lines in a file
int getLines(char* fileName){
	int i = 0;
	FILE *fp;

	fp = fopen(fileName, "r");

	if(fp == NULL){
		fprintf(stderr, "Error. Cannot find Manifest. \n");
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

void rmove(char* projectName, char* filePath){

	int found = 0; // changed to 1 when the file if found in the manifest

	//checks if the folder exists
	DIR *de;
	if ((de = opendir(projectName)) == NULL){
		fprintf(stderr, "Project %s does not exist\n", projectName);
		exit(0);
	}

	//make the path to the manifest
	char* manifestPath = (char*)malloc(strlen(projectName)+ strlen("/.Manifest"));
	memcpy(manifestPath, projectName, strlen(projectName));
	strcat(manifestPath, "/.Manifest");


	char* sentence = makeSentence(manifestPath); //all of manifest
	int length = getLines(manifestPath) - 1 ; //number of lines in manifest

	struct node array[length]; //array to hold everything
	int i = 0;

	char* version;

	//add everything to the array
	if(sentence != NULL){
		char* word = strtok(sentence, "\n");
		version = word;

		while(i < length){

			word = strtok(NULL, "\t");
			array[i].version = word;

			word = strtok(NULL, "\t");
			array[i].addr = word;

			if(strcmp(filePath, word) == 0){
				found = 1;  //file found in the manifest
			}

			word = strtok(NULL, "\n");
			array[i].hash = word;

			i++;
		}

	}

	//returns error if the file is not found
	if(found == 0){
		fprintf(stderr, "File not found at %s\n", filePath);
		return;
	}

	remove(manifestPath); //delete the current manifest

	//makes the new manifest writes everything but the one with the paths into the manifest back
	int fd = open(manifestPath, O_CREAT | O_RDWR, 0644);

	//write version on top of the manifest;
	write(fd, version, strlen(version));
	write(fd, "\n", strlen("\n"));

	for(i = 0; i < length; i++){
		if(strcmp(array[i].addr, filePath) != 0){
			write(fd, array[i].version, strlen(array[i].version));
			write(fd, "\t", strlen("\t"));
			write(fd, array[i].addr, strlen(array[i].addr));
			write(fd, "\t", strlen("\t"));
			write(fd, array[i].hash, strlen(array[i].hash));
			write(fd, "\n", strlen("\n"));

		}
	}

	close(fd);

	return;

}


//add flag
void add(char* projectName, char* filePath){

	int found = 0; // changed to 1 when the file if found in the manifest

	//checks if the folder exists
	DIR *de;
	if ((de = opendir(projectName)) == NULL){
		fprintf(stderr, "Project %s does not exist\n", projectName);
		exit(0);
	}

	//make the path to the manifest
	char* manifestPath = (char*)malloc(strlen(projectName)+ strlen("/.Manifest"));
	memcpy(manifestPath, projectName, strlen(projectName));
	strcat(manifestPath, "/.Manifest");


	char* sentence = makeSentence(manifestPath); //all of manifest in a string
	int length = (getLines(manifestPath)) - 1; //number of lines in manifest

	struct node array[length]; //array to hold everything
	int i = 0;

	char* version;

	//add everything to the array
	if(sentence != NULL){
		char* word = strtok(sentence, "\n");
		version = word;

		while(i < length){
			word = strtok(NULL, "\t");
			array[i].version = word;

			word = strtok(NULL, "\t");
			array[i].addr = word;

			if(strcmp(filePath, word) == 0){
				fprintf(stderr, "Updating file...\n");
				found = 1;

				//updates file version
				int temp = atoi(array[i].version);
				temp++;
				char snum[BUFF];
				snprintf(snum, sizeof(snum), "%d", temp);
				array[i].version = snum;

				//get new hashcode
				static unsigned char hashCode[65];
				sha256File(filePath, hashCode);
				word = strtok(NULL, "\n");
				array[i].hash = hashCode;

			} else {

				word =  strtok(NULL, "\n");
				array[i].hash = word;
			}

			i++;
		}

	}

	//case: updating a file
	if(found == 1){

		remove(manifestPath); //delete the current manifest

		//makes the new manifest writes everything but the one with the paths into the manifest back
		int fd = open(manifestPath, O_CREAT | O_RDWR, 0644);

		//write it on top of the manifest
		write(fd, version, strlen(version));
		write(fd, "\n", strlen("\n"));

		for(i = 0; i < length; i++){
			write(fd, array[i].version, strlen(array[i].version));
			write(fd, "\t", strlen("\t"));
			write(fd, array[i].addr, strlen(array[i].addr));
			write(fd, "\t", strlen("\t"));
			write(fd, array[i].hash, strlen(array[i].hash));
			write(fd, "\n", strlen("\n"));
		}

		//case: adding a new file
	} else {

		//get hashcode
		static unsigned char hashCode[65];
		sha256File(filePath, hashCode);

		//makes the new manifest writes everything but the one with the paths into the manifest back
		int fd = open(manifestPath, O_APPEND | O_RDWR, 0644);
		write(fd, "1", strlen("1"));
		write(fd, "\t", strlen("\t"));
		write(fd, filePath, strlen(filePath));
		write(fd, "\t", strlen("\t"));
		write(fd, hashCode, strlen(hashCode));
		write(fd, "\n", strlen("\n"));

		close(fd);

	}

	//free malloced variables
	free(manifestPath);

	return;


}

void update(char* projectName){

	char** config = readConfig();
	int sockfd = connectServer(config[0], strtol(config[1], NULL, 10));

	//sends message to server to look for the project
	char found[BUFF];
	sendMsg("update", sockfd);
	sendMsg(projectName, sockfd);
	recMsg(found, sockfd);

	//send an error if the project doesn't exist on the server side
	if(strcmp(found, "not found") == 0){
		fprintf(stderr, "\nError. Project %s does not exist in server \n", projectName);
		return;
	}

	//checks if the folder exists on client side
	DIR *de;
	if ((de = opendir(projectName)) == NULL){
		fprintf(stderr, "\nError. Project %s does not exist\n", projectName);
		return;
	}

	//gets the Manifest and lines from server
	char sManifest[BUFF]; //manifest in the encoded form
	char serverL[BUFF];
	recMsg(sManifest, sockfd);
	recMsg(serverL, sockfd); //number of lines in server file
	int sLines = atoi(serverL) - 1; //converts to an int

	//decodes the message
	char** serverManifest = decodeFile(sManifest);

	//make the path to the manifest client side
	char* manifestPath = (char*)malloc(strlen(projectName)+ strlen("/.Manifest"));
	memcpy(manifestPath, projectName, strlen(projectName));
	strcat(manifestPath, "/.Manifest");

	char* clientManifest = makeSentence(manifestPath); //client as a string
	int cLines = getLines(manifestPath) - 1; //number of lines in client

	//creates an .Update if it doesn't already exit
	char* updatePath = (char*)malloc(strlen(projectName) + strlen("/.Update"));
	memcpy(updatePath, projectName, strlen(projectName));
	strcat(updatePath, "/.Update");
	int fd = open(updatePath, O_CREAT | O_RDWR, 0644);

	//put all of a manifest in a array if not empty
	char* clientVersion;
	char* word =  strtok(clientManifest, "\n");
	clientVersion = word;

	struct node client[cLines];
	int k = 0;

	while(k < cLines){
		word = strtok(NULL, "\t");
		client[k].version = word ;

		word = strtok(NULL, "\t");
		client[k].addr = word;

		word =  strtok(NULL, "\n");
		client[k].hash = word;

		k++;
	}

	//put all of a manifest in a array if not empty
	char* serverVersion;
	char* temp =  strtok(serverManifest[1], "\n");
	serverVersion = temp;

	struct node server[sLines];
	int l = 0;

	while(l < sLines){
		temp = strtok(NULL, "\t");
		server[l].version = temp;

		temp = strtok(NULL, "\t");
		server[l].addr = temp;

		temp =  strtok(NULL, "\n");
		server[l].hash = temp;

		l++;
	}

	int fileFound;
	int i;

	int onlyServer[sLines]; //everything is zero
	memset(onlyServer, 0, sLines);//everything is zero

	//iterates through client manifest
	for(i = 0; i < cLines; i++){
		//get the live hash
		static unsigned char live[65];
		sha256File(client[i].addr, live);

		int j;
		fileFound = 0;

		//itersates through server manifest
		for(j = 0; j < sLines; j++){

			if(strcmp(client[i].addr, server[j].addr) == 0){
				fileFound = 1; //file in both client and server
				onlyServer[j] = 1;

				//MODIFY
				if((strcmp(server[j].version, client[i].version) != 0) && (strcmp(serverVersion, clientVersion) != 0) && (strcmp(live, client[i].hash) == 0)){
					//different version in manifest, different manifest versions and live & client hash are the same
					write(fd, "M", strlen("M"));
					write(fd, "\t", strlen("\t"));
					write(fd, server[j].version, strlen(server[j].version));
					write(fd, "\t", strlen("\t"));
					write(fd, server[j].addr, strlen(server[j].addr));
					write(fd, "\t", strlen("\t"));
					write(fd, server[j].hash, strlen(server[j].hash));
					write(fd, "\n", strlen("\n"));

					fprintf(stdout, "Modifying  %s \n", server[j].addr);
				}

				//UPLOAD
				if((strcmp(serverVersion, clientVersion) == 0) && (strcmp(live, server[j].hash) == 0)){
					write(fd, "U", strlen("U"));
					write(fd, "\t", strlen("\t"));
					write(fd, server[j].version, strlen(server[j].version));
					write(fd, "\t", strlen("\t"));
					write(fd, server[j].addr, strlen(server[j].addr));
					write(fd, "\t", strlen("\t"));
					write(fd, server[j].hash, strlen(server[j].hash));
					write(fd, "\n", strlen("\n"));
				}
			}
		}

		//DELETE
		if((strcmp(serverVersion, clientVersion) != 0) && (fileFound == 0)){
			write(fd, "D", strlen("D"));
			write(fd, "\t", strlen("\t"));
			write(fd, client[i].version, strlen(client[i].version));
			write(fd, "\t", strlen("\t"));
			write(fd, client[i].addr, strlen(client[i].addr));
			write(fd, "\t", strlen("\t"));
			write(fd, client[i].hash, strlen(client[i].hash));
			write(fd, "\n", strlen("\n"));

			fprintf(stdout, "Deleting %s \n", client[i].addr);
		}

		//UPLOAD
		if((strcmp(serverVersion, clientVersion) == 0) && (fileFound == 0)){
			write(fd, "U", strlen("U"));
			write(fd, "\t", strlen("\t"));
			write(fd, client[i].version, strlen(client[i].version));
			write(fd, "\t", strlen("\t"));
			write(fd, client[i].addr, strlen(client[i].addr));
			write(fd, "\t", strlen("\t"));
			write(fd, client[i].hash, strlen(client[i].hash));
			write(fd, "\n", strlen("\n"));
		}


	}

	//ADDING
	for(i = 0; i < sLines; i++){
		if((onlyServer[i] != 1) && (strcmp(serverVersion, clientVersion) != 0)){
			write(fd, "A", strlen("A"));
			write(fd, "\t", strlen("\t"));
			write(fd, server[i].version, strlen(server[i].version));
			write(fd, "\t", strlen("\t"));
			write(fd, server[i].addr, strlen(server[i].addr));
			write(fd, "\t", strlen("\t"));
			write(fd, server[i].hash, strlen(server[i].hash));
			write(fd, "\n", strlen("\n"));

			fprintf(stdout, "Adding  %s \n", server[i].addr);
		}
	}

	//NO UPDATES
	int uLines = getLines(updatePath); //number of lines in update
	if(uLines == 0){
		fprintf(stdout, "No updates in %s\n", projectName);
	}


	close(fd);




}

//connects client and server
int connectServer(char* ip, int port) {
	//printf("IP: %s\n", ip);
	//printf("port: %d\n", port);

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



	while(1) {
		printf("Attempting to connect...\n");
		if(connect(sock, (struct sockaddr*) &serverAddr, sizeof(serverAddr)) < 0) {
			fprintf(stderr, "Error. Connection to server failed.\n");
		} else {
			printf("Successfully connected to server!\n");
			return sock;
		}
		sleep(3);
	}


}

void configure(char* ip, char* port) {
	int intPort = strtol(port, NULL, 10);
	if(intPort < 8000 || intPort > 65535) {
		fprintf(stderr, "Invalid port. Need to be >8k and <64k.\nTerminating.\n");
		exit(0);
	}
	int fd;

	remove(".configure");
	fd = open(".configure", O_CREAT | O_RDWR, 0644);
	write(fd, ip, strlen(ip));
	write(fd, " ", 1);
	write(fd, port, strlen(port));
	close(fd);


}



void create(char* projectName) {
	char** config = readConfig();
	int sockfd = connectServer(config[0], strtol(config[1], NULL, 10));

	//printf("returned socket.\n");
	//printf("SOCKFD: %d\n\n", sockfd);
	sendMsg("create", sockfd);
	//printf("create message sent.\n");
	sendMsg(projectName, sockfd);
	//printf("project name message sent.\n");

	char file[BUFF];
	bzero(file, BUFF);
	recMsg(file, sockfd);
	//printf("received message\n");

	//printf("status: %s\n\n", file);

	if(strlen(file) != 0) {
		printf("Project directory succesfully made on server! Creating directories locally...\n");

		char* filePath = (char*) malloc(strlen(projectName) * sizeof(char));
		filePath = projectName;
		mkdir(filePath, 0700);

		char** recFile = decodeFile(file);

		sprintf(filePath, "%s%s", filePath, recFile[0]);
		FILE *fd = fopen(filePath, "w");
		fprintf(fd, "1\n");
		fclose(fd);

	} else {
		fprintf(stderr, "Error. Project with name already exists!\n");
		exit(0);
	}

	close(sockfd);
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

//parses sentence into array
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

int folderExist(char* lookingFor){
	struct stat buffer;
	stat(lookingFor, &buffer);

	if (S_ISDIR(buffer.st_mode)){
		return 1;
	}
	return 0;

}

void checkout(char* projectName) {
	char** config = readConfig();
	int sockfd = connectServer(config[0], strtol(config[1], NULL, 10));

	if(folderExist(projectName) != 0) {
		fprintf(stderr, "Error. Project already exists at the client! Exiting...\n");
		exit(0);
	}

	sendMsg("checkout", sockfd);
	sendMsg(projectName, sockfd);

	char msg[BUFF];
	//for some reason checkout is sending "exists" twice from server...
	recMsg(msg, sockfd);
	recMsg(msg, sockfd);

	if(strcmp("exists", msg) == 0) {
		printf("Project exists at server. Continuing...\n");

		char compMsg[BUFF];
		recMsg(compMsg, sockfd);

		char* compStr = compMsg;
		int count = wordCount(compStr);
		char** arr = makeArr(compStr, count, "\n");

		mkdir(projectName, 0700);
		printf("Decompressing and extracting the following files and directories to %s in client...\n\n", projectName);

		int i;
		char* currFile;
		FILE* fd;
		for(i = 0; arr[i+1] != NULL; i++) {
			count = wordCount(arr[i]);
			char* currLine = (char*) malloc(strlen(arr[i]) * sizeof(char));
			currLine = arr[i];
			if(strstr(currLine, ".server_repo/") != NULL) {
				currLine += strlen(".server_repo/");
			}
			char buffer[BUFF];
			bzero(buffer, BUFF);
			sprintf(buffer, arr[i], strlen(arr[i]));
			char** currArr = makeArr(arr[i], count, " \t");

			if(count == 3 && strcmp(currArr[1], "R") == 0) {
				printf("Making file: %s\n", currLine);
				currFile = currLine;
				fd = fopen(currFile, "w");
			} else if (count == 3 && strcmp(currArr[1], "D") == 0){
				printf("Making directory: %s\n", currLine);
				mkdir(currLine, 0700);
			} else {
				fprintf(fd, "%s\n", buffer);
			}
		}
		fclose(fd);
		printf("Success! Compressed files unzipped and brought to client. Removing compressed file at server...\n");
		exit(0);
	} else if(strcmp("no path", msg) == 0) {
		fprintf(stderr, "Project does not exist in server! Exiting...\n");
		exit(0);
	}
}



void commit(char* projectName){

	char** config = readConfig();
	int sockfd = connectServer(config[0], strtol(config[1], NULL, 10));

	//sends message to server to look for the project
	char found[BUFF];
	bzero(found, BUFF);
	sendMsg("commit", sockfd);
	sendMsg(projectName, sockfd);
	recMsg(found, sockfd);

	//send an error if the project doesn't exist on the server side
	if(strcmp(found, "not found") == 0){
		fprintf(stderr, "\nError. Project %s does not exist in server \n", projectName);
		return;
	}

	//checks if the folder exists on client side
	DIR *de;
	if ((de = opendir(projectName)) == NULL){
		fprintf(stderr, "\nError. Project %s does not exist\n", projectName);
		closedir(de);
		return;
	}

	//creates an .Update if it doesn't already exit
	char* updatePath = (char*)malloc(strlen(projectName) + strlen("/.Update"));
	memcpy(updatePath, projectName, strlen(projectName));
	strcat(updatePath, "/.Update");

	//returns error if the .update isn't empty
	FILE *file;
	if(access(updatePath, F_OK)){
		fclose(file);
		int lines = getLines(updatePath);

		if(lines > 0){
			fprintf(stderr, "\nError. Ugrade %s first. \n", projectName);
			return;

		}
	}

	//gets the Manifest and lines from server
	char sManifest[BUFF]; //manifest in the encoded form
	bzero(sManifest, BUFF);
	char serverL[BUFF];
	bzero(serverL, BUFF);
	recMsg(sManifest, sockfd);
	recMsg(serverL, sockfd); //number of lines in server file
	int sLines = atoi(serverL) - 1; //converts to an int

	//decodes the message
	char** serverManifest = decodeFile(sManifest);

	//make the path to the manifest client side
	char* manifestPath = (char*)malloc(strlen(projectName)+ strlen("/.Manifest"));
	memcpy(manifestPath, projectName, strlen(projectName));
	strcat(manifestPath, "/.Manifest");

	char* clientManifest = makeSentence(manifestPath); //client as a string
	int cLines = getLines(manifestPath) - 1; //number of lines in client

	//creates an .Update if it doesn't already exit
	char* commitPath = (char*)malloc(strlen(projectName) + strlen("/.Commit"));
	memcpy(commitPath , projectName, strlen(projectName));
	strcat(commitPath , "/.Commit");
	int fd = open(commitPath, O_CREAT | O_RDWR, 0644);

	//put all of client manifest in a array if not empty
	char* clientVersion;
	char* word =  strtok(clientManifest, "\n");
	clientVersion = word;

	struct node client[cLines];
	int k = 0;

	while(k < cLines){
		word = strtok(NULL, "\t");
		client[k].version = word ;

		word = strtok(NULL, "\t");
		client[k].addr = word;

		word =  strtok(NULL, "\n");
		client[k].hash = word;

		k++;
	}

	//put all of server manifest in a array if not empty
	char* serverVersion;
	char* temp =  strtok(serverManifest[1], "\n");
	serverVersion = temp;

	struct node server[sLines];
	int l = 0;

	while(l < sLines){
		temp = strtok(NULL, "\t");
		server[l].version = temp;

		temp = strtok(NULL, "\t");
		server[l].addr = temp;

		temp =  strtok(NULL, "\n");
		server[l].hash = temp;

		l++;
	}

	//coverts them to numbers to compare version
	int s = atoi(serverVersion);
	int c = atoi(clientVersion);

	if(s!=c){
		fprintf(stderr, "Update %s before trying to commit\n", projectName);
		return;
	}

	int fileFound;
	int i;

	int onlyServer[sLines]; //everything is zero
	memset(onlyServer, 0, sLines);//everything is zero

	for(i = 0; i < cLines; i++){
		fileFound = 0;

		//get the live hash
		static unsigned char live[65];
		sha256File(client[i].addr, live);

		int j;
		for(j = 0; j < sLines; j++){
			if(strcmp(server[j].addr, client[i].addr) == 0){

				fileFound = 1;
				onlyServer[j] = 1;

				int sver = atoi(server[j].version);
				int cver = atoi(client[i].version);

				//conflicts error
				if((strcmp(server[j].hash, client[i].hash) != 0) && (sver > cver)){
					fprintf(stderr, "Synch %s with the repository before committing changes.\n", projectName);
					close(fd);
					remove(commitPath);
					return;
				}

				//CASE 2
				if((strcmp(live, client[i].hash) != 0) || (strcmp(live, server[j].hash) != 0)){

					//increment verison file if file in direcotry is different than client
					if((strcmp(live, client[i].hash) != 0)){
						int hold = atoi(client[i].version);
						hold++;

						char snum[BUFF];
						bzero(snum, BUFF);
						snprintf(snum, sizeof(snum), "%d", hold);
						client[i].version = snum;

					}

					write(fd, "U", strlen("U"));
					write(fd, "\t", strlen("\t"));
					write(fd, client[i].version, strlen(client[i].version));
					write(fd, "\t", strlen("\t"));
					write(fd, client[i].addr, strlen(client[i].addr));
					write(fd, "\t", strlen("\t"));
					write(fd, live, strlen(live));
					write(fd, "\n", strlen("\n"));
				}

			}
		}

		//ONLY IN CLIENT --> ADD
		if(fileFound == 0){
			write(fd, "A", strlen("A"));
			write(fd, "\t", strlen("\t"));
			write(fd, client[i].version, strlen(client[i].version));
			write(fd, "\t", strlen("\t"));
			write(fd, client[i].addr, strlen(client[i].addr));
			write(fd, "\t", strlen("\t"));
			write(fd, client[i].hash, strlen(client[i].hash));
			write(fd, "\n", strlen("\n"));
		}

	}


	//ONLY IN SERVER --> DELETE
	for(i = 0; i<sLines; i++){

		if(onlyServer[i] == 0){
			write(fd, "D", strlen("D"));
			write(fd, "\t", strlen("\t"));
			write(fd, server[i].version, strlen(server[i].version));
			write(fd, "\t", strlen("\t"));
			write(fd, server[i].addr, strlen(server[i].addr));
			write(fd, "\t", strlen("\t"));
			write(fd, server[i].hash, strlen(server[i].hash));
			write(fd, "\n", strlen("\n"));
		}
	}

	close(fd);

	//sends commit over without the extra char
	FILE* fp = fopen(commitPath, "r");
	int x = 0;
	char commit[BUFF];
	bzero(commit, BUFF);
	while((x= fread(commit, 1, sizeof(commit), fp)) >0){
		sprintf(commit, "%s", commit);
	}

	fclose(fp);

	//sends commit to server
	sendMsg(commit, sockfd);


	return;

}


void currentVersion(char* projectName) {
	char** config = readConfig();
	int sockfd = connectServer(config[0], strtol(config[1], NULL, 10));

	sendMsg("currentversion", sockfd);
	sendMsg(projectName, sockfd);

	char msg[BUFF];
	recMsg(msg, sockfd);

	if(strcmp("folder exists", msg) == 0) {
		printf("Folder exists at server! Continuing...\n");
		char manifest[BUFF];
		recMsg(manifest, sockfd);

		char * curLine = manifest;
		char** currLineArr;

		printf("CURRENT VERSION OF PROJECT FILES AT SERVER\n==========================================\n");
		while(curLine)
		{
			char * nextLine = strchr(curLine, '\n');
			if (nextLine) *nextLine = '\0';  // temporarily terminate the current line
			int count = wordCount(curLine);
			currLineArr = makeArr(curLine, count, "\t");
			if(count == 1) {
				printf("%s/.Manifest\tver. %s\n", projectName, currLineArr[0]);
			} else if (count == 3){
				printf("%s/%s\tver. %s\n", projectName, currLineArr[1], currLineArr[0]);
			}

			if (nextLine) *nextLine = '\n';
			curLine = nextLine ? (nextLine+1) : NULL;
		}
	} else {
		fprintf(stderr, "Error. Project does not exist at server.\nTerminating...\n");
		exit(0);
	}
}


void destroy(char* projectName) {
	char** config = readConfig();
	int sockfd = connectServer(config[0], strtol(config[1], NULL, 10));

	sendMsg("destroy", sockfd);
	sendMsg(projectName, sockfd);

	char msg[BUFF];
	recMsg(msg, sockfd);

	if(strcmp("folder exists", msg) == 0) {
		printf("\nSuccess! Project at server removed!\n");
	} else {
		fprintf(stderr, "Error. Project does not exist at server.\nTerminating...\n");
		exit(0);
	}



}


void history(char* projectName) {
	char** config = readConfig();
	int sockfd = connectServer(config[0], strtol(config[1], NULL, 10));

	sendMsg("history", sockfd);
	sendMsg(projectName, sockfd);

	char msg[BUFF];
	recMsg(msg, sockfd);

	if(strcmp("folder exists", msg) == 0) {
		char history[BUFF];
		recMsg(history, sockfd);
		printf("\nSuccess! Printing history of project\n====================================\n%s", history);
	} else {
		fprintf(stderr, "Error. Project does not exist at server.\nTerminating...\n");
		exit(0);
	}
}


void upgrade(char* projectName) {
	char** config = readConfig();
	int sockfd = connectServer(config[0], strtol(config[1], NULL, 10));

	sendMsg("upgrade", sockfd);
	sendMsg(projectName, sockfd);

	char msg[BUFF];
	recMsg(msg, sockfd);

	char updateBuff[BUFF];
	if(strcmp("folder exists", msg) == 0) {
		printf("Folder exists at server! Continuing...\n\n");
		sprintf(updateBuff, "%s%s", projectName, "/.Update", strlen(projectName), strlen("/.Update"));
		printf("Update: %s\n\n", updateBuff);

		if(access(updateBuff, F_OK) != -1) {
			int size;
			FILE* fp = fopen(updateBuff, "r");
			if (NULL != fp) {
				fseek (fp, 0, SEEK_END);
				size = ftell(fp);

				if (size == 0) {
					printf("No upates. Everything is up to date!\n");
					exit(0);
				}
			}
		} else {
			fprintf(stderr, "Error. Update does not exist!\n");
			exit(0);
		}
	} else {
		fprintf(stderr, "Error. Project does not exist at server.\nTerminating...\n");
		exit(0);
	}

	int i;
	char* currFile;

	char manPath[BUFF];
	bzero(manPath, BUFF);
	sprintf(manPath, "%s/.Manifest", projectName);

	FILE* manfd = fopen(manPath, "r");
	int x = 0;
	char manifest[BUFF];
	bzero(manifest, BUFF);
	while((x = fread(manifest, 1, sizeof(manifest), manfd)) > 0) {
		sprintf(manifest, "%s", manifest);
	}
	fclose(manfd);

	char newMan[BUFF];
	bzero(newMan, BUFF);

	FILE* fp = fopen(updateBuff, "r");
	x = 0;
	char buffer[BUFF];
	bzero(buffer, BUFF);

	while((x = fread(buffer, 1, sizeof(buffer), fp)) > 0) {
		sprintf(buffer, "%s", buffer);
	}
	fclose(fp);

	char* compStr = buffer;
	int count = wordCount(compStr);
	char** arr = makeArr(compStr, count, "\n");
	char** currArr;

	char * curLine = manifest;
	count = wordCount(curLine);
	char** manifest2d = makeArr(curLine, count, "\n");
	char** currLineArr;

	int j;

	int ver = atoi(manifest2d[0]);
	ver++;
	char verBuff[BUFF];
	bzero(verBuff, BUFF);
	snprintf(verBuff, sizeof(verBuff), "%d", ver);
	sprintf(newMan, "%s\n", verBuff, strlen(verBuff), strlen("\n"));

	for(i = 0; arr[i] != NULL; i++)  {
		count = wordCount(arr[i]);
		char buffer[BUFF];
		bzero(buffer, BUFF);
		sprintf(buffer, arr[i], strlen(arr[i]));
		currArr = makeArr(buffer, count, "\t");
		if(strcmp(currArr[0], "A") == 0) {
			sendMsg(currArr[2], sockfd);
			char encode[BUFF];
			bzero(encode, BUFF);
			recMsg(encode, sockfd);

			char** decoded = decodeFile(encode);

			char buildLine[BUFF];
			bzero(buildLine, BUFF);

			char sha[65];
			shaString(decoded[1], sha);

			sprintf(buildLine, "%s\t%s\t%s", currArr[1], decoded[0], sha, strlen(currArr[1]), strlen(decoded[0]), strlen(sha));
			sprintf(newMan, "%s%s\n", newMan, buildLine, strlen(newMan), strlen(buildLine));
		}
	}

	for(j = 1; manifest2d[j+1] != NULL; j++) {
		int flag = 0;
		int count = wordCount(manifest2d[j]);
		char manBuff[BUFF];
		sprintf(manBuff, "%s", manifest2d[j], strlen(manifest2d[j]));
		char** currLineArr = makeArr(manBuff, count, "\t");

		char dCom[BUFF];
		for(i = 0; arr[i] != NULL; i++) {
			count = wordCount(arr[i]);
			char buffer[BUFF];
			bzero(buffer, BUFF);
			sprintf(buffer, arr[i], strlen(arr[i]));
			currArr = makeArr(buffer, count, "\t");

			if(strcmp(currArr[0], "D") == 0) {
				sprintf(dCom, "%s", currArr[2], strlen(currArr[1]));
				break;
			}

			if (strcmp(currArr[0], "M") == 0){
				if(strcmp(currArr[2], currLineArr[1]) == 0) {
					sendMsg(currArr[2], sockfd);
					char encode[BUFF];
					bzero(encode, BUFF);
					recMsg(encode, sockfd);

					char** decoded = decodeFile(encode);

					char buildLine[BUFF];
					bzero(buildLine, BUFF);

					char sha[65];
					shaString(decoded[1], sha);

					sprintf(buildLine, "%s\t%s\t%s", currArr[1], decoded[0], sha, strlen(currArr[1]), strlen(decoded[0]), strlen(sha));
					sprintf(newMan, "%s%s\n", newMan, buildLine, strlen(newMan), strlen(buildLine));
					flag = 1;


					remove(decoded[0]);
					printf("content: %s\n\n", decoded[1]);
					FILE* fd = fopen(decoded[0], "w");
					fprintf(fd, decoded[1]);
					fclose(fd);
					break;
				} else if(strcmp(currArr[0], "D") == 0) {
					if(strcmp(currArr[2], currLineArr[1]) == 0) {
						flag = 1;
						break;
					}
				}

			}

		}
		if(flag == 0 && strcmp(dCom, currLineArr[1]) != 0) {
			char buildLine[BUFF];
			bzero(buildLine, BUFF);


			sprintf(buildLine, "%s\t%s\t%s", currLineArr[0], currLineArr[1], currLineArr[2], strlen(currLineArr[0]), strlen(currLineArr[1]), strlen(currLineArr[2]));
			sprintf(newMan, "%s%s\n", newMan, buildLine, strlen(newMan), strlen(buildLine));
			bzero(dCom, BUFF);
		}
	}

	remove(manPath);
	printf("NEW MAN\n==========================\n%s", newMan);
	printf("path: %s\n", manPath);
	FILE* fd = fopen(manPath, "w");
	fprintf(fd, newMan);
	fclose(fd);
	sendMsg("NickAndVancha'sUniqueKeyThatShouldn'tBeAFileToTerminateAndFinish", sockfd);

	remove(updateBuff);
}

void push(char* projectName) {
	char updateBuff[BUFF];
	bzero(updateBuff, BUFF);
	sprintf(updateBuff, "%s%s", projectName, ".Update", strlen(projectName), strlen(".Update"));
	if(access(updateBuff, F_OK) != -1) {
		int size;
		FILE* fp = fopen(updateBuff, "r");
		if (NULL != fp) {
			fseek (fp, 0, SEEK_END);
			size = ftell(fp);

			if (size == 0) {
				printf("No updates. Everything is up to date!\n");
			} else if (size > 0) {
				printf("Updates are pending. Checking for anything modified...\n");
				FILE* fp = fopen(updateBuff, "r");
				int x = 0;
				char updateStr[BUFF];
				bzero(updateStr, BUFF);

				while((x = fread(updateStr, 1, sizeof(updateStr), fp)) > 0) {
					sprintf(updateStr, "%s", updateStr);
				}
				fclose(fp);

				if(strstr(updateStr, "M\t") != NULL) {
					fprintf(stderr, "Files were modified since last push. Upgrade first!\n");
					exit(0);
				} else {
					printf("M does not exist.\n");
				}
			}
		}
	}

	char** config = readConfig();
	int sockfd = connectServer(config[0], strtol(config[1], NULL, 10));

	sendMsg("push", sockfd);
	sendMsg(projectName, sockfd);
	char msg[BUFF];
	recMsg(msg, sockfd);

	if(strcmp("folder exists", msg) == 0) {
		printf("Folder exists at server! Continuing...\n\n");
		char commitPath[BUFF];
		bzero(commitPath, BUFF);


		//printf("TEST\n");
		sprintf(commitPath, "%s%s", projectName, "/.Commit", strlen(projectName), strlen("/.Commit"));
		char* encodedCom = encodeFile(commitPath, ".Commit");

		FILE* fp = fopen(commitPath, "r");
		int x = 0;
		char commitContent[BUFF];
		bzero(commitContent, BUFF);


		while((x = fread(commitContent, 1, sizeof(commitContent), fp)) > 0) {
			sprintf(commitContent, "%s", commitContent);
		}
		fclose(fp);

		sendMsg(commitContent, sockfd);

		bzero(msg, BUFF);
		recMsg(msg, sockfd);

		if(strcmp("success", msg) == 0) {
			printf("success!\n");
		} else {
			fprintf(stderr,"Error. Push failed.\n");
			exit(0);
		}

		return;




	} else {
		fprintf(stderr, "Error. Project does not exist at server.\nTerminating...\n");
		exit(0);
	}


}


void rollback(char* projectName, char* ver) {
	char** config = readConfig();
	int sockfd = connectServer(config[0], strtol(config[1], NULL, 10));


	sendMsg("rollback", sockfd);
	sendMsg(projectName, sockfd);
	char msg[BUFF];
	bzero(msg, BUFF);
	recMsg(msg, sockfd);

	if(strcmp("folder exists", msg) == 0) {
		printf("Folder exists at server! Continuing...\n\n");
		sendMsg(ver, sockfd);

		char res[BUFF];
		bzero(res, BUFF);

		recMsg(res, sockfd);

		if(strcmp(res, "success") == 0) {
			printf("Successful roll back!\n");
		} else {
			fprintf(stderr, "Error. Invalid rollback version.\n");
		}



	} else {
		fprintf(stderr, "Error. Project does not exist at server.\nTerminating...\n");
		exit(0);
	}




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
			checkout(argv[2]);
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
	} else if (strcmp("commit", argv[1]) == 0) {
		if(access(".configure", F_OK) != -1) {
			//printf("commit!\n");
			commit(argv[2]);
		} else {
			fprintf(stderr, "Error. Configurations not set! No valid IP and port.\n");
			return 0;
		}
	} else if (strcmp("currentversion", argv[1]) == 0) {
		if(access(".configure", F_OK) != -1) {
			printf("currentversion\n");
			currentVersion(argv[2]);
		} else {
			fprintf(stderr, "Error. Configurations not set! No valid IP and port.\n");
			return 0;
		}
	} else if (strcmp("destroy", argv[1]) == 0) {
		if(access(".configure", F_OK) != -1) {
			destroy(argv[2]);
		}
	} else if (strcmp("history", argv[1]) == 0) {
		if(access(".configure", F_OK) != -1) {
			history(argv[2]);
		}
	} else if (strcmp("add", argv[1]) == 0) {
		add(argv[2], argv[3]);

	}  else if (strcmp("remove", argv[1]) == 0) {
		rmove(argv[2], argv[3]);

	}  else if (strcmp("update", argv[1]) == 0) {
		if(access(".configure", F_OK) != -1) {
			update(argv[2]);
		} else {
			fprintf(stderr, "Error. Configurations not set! no Valid IP and port.\n");
		}
	} else if (strcmp("upgrade", argv[1]) == 0) {
		if(access(".configure", F_OK) != -1) {
			upgrade(argv[2]);
		} else {
			fprintf(stderr, "Error. Configurations not set! no Valid IP and port.\n");
		}

	} else if(strcmp("push", argv[1]) == 0) {
		if(access(".configure", F_OK) != -1) {
			push(argv[2]);
		} else {
			fprintf(stderr, "Error. Configurations not set! no Valid IP and port.\n");
		}
	} else if (strcmp("rollback", argv[1]) == 0) {
		if(access(".configure", F_OK) != -1 && argc == 4) {
			rollback(argv[2], argv[3]);
		} else {
			fprintf(stderr, "Error. Invalid number of inputs or configurations not set. no Valid IP and port.\n");
		}
	}

	return 0;
}
