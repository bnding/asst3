#include <stdlib.h>
#include <stdio.h>
#include <signal.h>
#include <sys/types.h>


int main(int argc, char** argv) {
	if(argc != 2) {
		printf("usage: kill pid\n");
		return 0;
	}

	int pid = atoi(argv[1]);
	if (kill(pid, SIGUSR2) == 0) {
		printf("signal sent to %d\n", pid);
	} else {
		perror("kill");
	}

	return 0;



}
