all: WTF WTFserver

WTF: client.c sha256.h ftp.h mtp.h
	gcc -O -g client.c -o WTF -lcrypto -lz

WTFserver: server.c sha256.h ftp.h mtp.h
	gcc -O -g server.c -o WTFserver -lpthread -lz

clean:
	rm-rf WTF WTFserver 
