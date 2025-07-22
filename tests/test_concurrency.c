#include "../src/descriptor_table.h"
#include <pthread.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <fcntl.h>

#define THREADS 12
// Message will include thread number

void* thread_func(void* arg) {
    int thread_id = *(int*)arg;
    char fname[32];
    snprintf(fname, sizeof(fname), "thread_file_%d.txt", thread_id);
    int fd = user_open(fname, O_CREAT | O_RDWR | O_TRUNC, 0644);
    if (fd < 0) {
        perror("user_open");
        return NULL;
    }
    char msg[64];
    snprintf(msg, sizeof(msg), "Hello from thread %d!\n", thread_id);
    user_write(fd, msg, strlen(msg));
    printf("Thread %d wrote: %s", thread_id, msg);
    lseek(fd, 0, SEEK_SET);
    char buf[128] = {0};
    ssize_t n = user_read(fd, buf, sizeof(buf)-1);
    if (n > 0) {
        printf("Thread %d read: %s", thread_id, buf);
    }
    user_close(fd);
    return NULL;
}

int main() {
    user_fd_init();
    pthread_t threads[THREADS];
    int thread_ids[THREADS];
    for (int i = 0; i < THREADS; i++) {
        thread_ids[i] = i;
        pthread_create(&threads[i], NULL, thread_func, &thread_ids[i]);
    }
    for (int i = 0; i < THREADS; i++) {
        pthread_join(threads[i], NULL);
    }
    user_fd_destroy();
    printf("Simple concurrency test complete.\n");
    return 0;
}
