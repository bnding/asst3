all: WTF

WTF: tempFile.c
	gcc -O -g tempFile.c -o WTF

clean:
	rm-rf WTF
