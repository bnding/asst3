#include <openssl/sha.h>
#include "zlib.h"

void hashString (unsigned char hash[SHA256_DIGEST_LENGTH], char hashBuff[65]) {
	int i;
	for(i = 0; i < SHA256_DIGEST_LENGTH; i++) {
		sprintf(hashBuff + (i*2), "%02x", hash[i]);
	}

	hashBuff[64] = 0;
}

void shaString (char* string, char hashBuff[65]) {
	unsigned char hash[SHA256_DIGEST_LENGTH];
	SHA256_CTX sha256;
	SHA256_Init(&sha256);
	SHA256_Update(&sha256, string, strlen(string));
	SHA256_Final(hash, &sha256);

	int i;
	for(i = 0; i < SHA256_DIGEST_LENGTH; i++) {
		sprintf(hashBuff + (i*2), "%02x", hash[i]);
	}
	hashBuff[64] = 0;
}

int sha256File(char* filePath, char hashBuff[65]) {
	int fd = open(filePath, O_RDONLY, 0644);
	if(fd == -1) {
		fprintf(stderr, "File does not exist at %s\n", filePath);
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

	shaString(content, hashBuff);

}
