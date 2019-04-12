#include <stdlib.h>
#include <stdio.h>
#include <unistd.h>
#include <pthread.h>

void *myThread(void *vargp) {
	sleep(1);
	printf("thread workload\n");
	return NULL;
}

int main() {
	pthread_t thread_id;
	printf("Before Thread\n");
	pthread_create(&thread_id, NULL, myThread, NULL);
	pthread_join(thread_id, NULL);
	printf("After Thread\n");
	exit(0);

	return 0;
}
