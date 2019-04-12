all: WTF WTFserver

WTF: client.c
	gcc -O -g client.c -o WTF

WTFserver: server.c
	gcc -O -g server.c -o WTFserver -lpthread

clean:
	rm-rf WTF WTFserver
