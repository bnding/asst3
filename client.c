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

//connects client and server
int connectServer(char* ip, int port) {
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
	if(intPort < 8000 || intPort > 65535) {
		fprintf(stderr, "Invalid port. Need to be >8k and <64k.\nTerminating.\n");
		exit(0);
	}
	int fd;

	fd = open(".configure", O_CREAT | O_RDWR, 0644);
	write(fd, ip, strlen(ip));
	write(fd, " ", 1);
	write(fd, port, strlen(port));
	close(fd);


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
		configStr[i] = c;
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
	char** config = readConfig();
	int sockfd = connectServer(config[0], strtol(config[1], NULL, 10));

	sendMsg("create", sockfd);
	sendMsg(projectName, sockfd);

	char file[BUFF];
	bzero(file, BUFF);
	recMsg(file, sockfd);

	printf("status: %s\n\n", file);

	if(strlen(file) != 0) {
		printf("Project directory succesfully made on server! Creating directories locally...\n");

		char* filePath = (char*) malloc(strlen(projectName) * sizeof(char));
		filePath = projectName;
		mkdir(filePath, 0700);

		char** recFile = decodeFile(projectName, file);

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

void traverse(char** arr, int numWords){
	int i;
	for(i = 0; i < numWords; i++){
		printf("%s\n", arr[i]);
	}
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
		printf("Decompressing and extracting the following files and directories to %s in client...\n", projectName);

		int i;
		char* currFile;
		FILE* fd;
		for(i = 0; arr[i+1] != NULL; i++) {
			count = wordCount(arr[i]);
			char* currLine = (char*) malloc(strlen(arr[i]) * sizeof(char));
			currLine = arr[i];
			if(strstr(currLine, ".server_repo/") != NULL) {
				currLine += strlen(".server_repo/");
				printf("file or dir: %s\n\n", currLine);
			}
			char buffer[BUFF];
			bzero(buffer, BUFF);
			sprintf(buffer, arr[i], strlen(arr[i]));
			char** currArr = makeArr(arr[i], count, " ");
			printf("currLine: %s\n", buffer);

			if(count == 3 && strcmp(currArr[1], "R") == 0) {
				printf("file: %s\n", currLine);
				currFile = currLine;
				fd = fopen(currFile, "w");
			} else if (count == 3 && strcmp(currArr[1], "D") == 0){
				printf("directory: %s\n", currLine);
				mkdir(currLine, 0700);
			} else {
				printf("buffer: %s\n", buffer);
				fprintf(fd, "%s\n", buffer);
			}
		}
		fclose(fd);
		printf("Success! Compressed files unzipped and brought to client.\n");
		exit(0);
	} else if(strcmp("no path", msg) == 0) {
		fprintf(stderr, "Project does not exist in server! Exiting...\n");
		exit(0);
	}
}



void commit(char* projectName) {
	char** config = readConfig();
	int sockfd = connectServer(config[0], strtol(config[1], NULL, 10));

	printf("commit\n");
	sendMsg("commit", sockfd);
	sendMsg(projectName, sockfd);

	char msg[BUFF];
	recMsg(msg, sockfd);

	if(strcmp("folder exists", msg) == 0) {
		printf("folder exists!\n");


	} else {
		fprintf(stderr, "Error. Folder does not exist at server.\nTerminating...\n");
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
			printf("commit!\n");
			commit(argv[2]);
		} else {
			fprintf(stderr, "Error. Configurations not set! No valid IP and port.\n");
			return 0;
		}
	}
	return 0;
}
