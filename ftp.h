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

#define delim :
#define BUFF 8192

//file:content
char* encodeFile(char* filePath, char* fileName) {
	//we already have projectName location on both client and server side. Just need the file name.


	int fd = open(filePath, O_RDONLY, 0644);
	if(fd == -1) {
		close(fd); 
		fprintf(stderr, "File does not exist.");
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


	char* msg = (char*) malloc(strlen(fileName) + strlen(content));;
	strcpy(msg, fileName);
	sprintf(msg, "%s:%s", msg, content);
	// printf("encoded message: %s\n", msg);
	return msg;
}


char** decodeFile(char* msg) {

	char* token;
	token = strtok(msg, ":");
	char** decoded = (char**) malloc(2 * sizeof(char*));

	decoded[0] = (char*) malloc(strlen(token) * sizeof(char));
	decoded[0] = token;

	token = strtok(NULL, ":");
	decoded[1] = (char*) malloc(strlen(token) * sizeof(char));
	decoded[1] = token;

	// printf("file name: %s\n", decoded[0]);
	// printf("file content: %s\n", decoded[1]);
	return decoded;
}
