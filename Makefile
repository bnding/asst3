all: WTF WTFserver

WTF: client.c sha256.h
	gcc -O -g client.c -o WTF -lcrypto 

WTFserver: server.c sha256.h
	gcc -O -g server.c -o WTFserver -lpthread

clean:
	rm-rf WTF WTFserver 
