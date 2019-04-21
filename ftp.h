#define delim :
#define BUFF 8192

//file:content
void sendOne(char* filePath, char* fileName, int socket) {
	//we already have projectName location on both client and server side. Just need the file name.

	int fd = open(filePath, O_RDONLY, 0644);
	if(fd == -1) {
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


	char msg[BUFF];
	bzero(msg, BUFF);
	int fileLen = strlen(filePath);
	strcpy(msg, fileName);
	sprintf(msg, "%s:%s", msg, content);
	printf("test\n\n");
	int n = write(socket, msg, strlen(msg));
	if (n < 0) {
		fprintf(stderr, "Error. Cannot write to socket\n");
		exit(0);
	}
	printf("test\n\n");
}


void recvOne(char* projectPath, int socket) {
	char msg[BUFF];
	bzero(msg, BUFF);
	read(socket, msg, BUFF);
	printf("%s\n\n", msg);


}
