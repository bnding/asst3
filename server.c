#include <stdlib.h>
#include <stdio.h>
#include <unistd.h>
#include <pthread.h>

void *myThread(void *vargp) {
	int i = 0;
	while(1) {
		printf("thread workload: %d\n", i);
		i++;
		sleep(1);
	}
	return NULL;
}

int main() {
	pthread_t thread_id;
	printf("Before Thread\n");
	pthread_create(&thread_id, NULL, myThread, NULL);
	pthread_join(thread_id, NULL);
	printf("After thread\n");
	exit(0);

	return 0;
}
