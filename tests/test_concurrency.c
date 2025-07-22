#include "../src/descriptor_table.h"
#include <stdio.h>
#include <pthread.h>
#include <fcntl.h>
#include <string.h>
#include <unistd.h>

void* thread_func(void* arg) {
    int tid = *(int*)arg;
    char fname[32];
    snprintf(fname, sizeof(fname), "thread_file_%d.txt", tid);
    int fd = user_open(fname, O_CREAT | O_RDWR, 0644);
    char msg[64];
    snprintf(msg, sizeof(msg), "Thread %d data", tid);
    user_write(fd, msg, strlen(msg));
    lseek(fd, 0, SEEK_SET); // Reset file offset for reading
    char buf[64] = {0};
    user_read(fd, buf, sizeof(buf)-1);
    printf("Thread %d wrote and read: '%s'\n", tid, buf);
    user_close(fd);
    return NULL;
}

int main() {
    user_fd_init();
    pthread_t threads[10];
    int tids[10];
    for (int i = 0; i < 10; i++) {
        tids[i] = i;
        pthread_create(&threads[i], NULL, thread_func, &tids[i]);
    }
    for (int i = 0; i < 10; i++) {
        pthread_join(threads[i], NULL);
    }
    user_fd_destroy();
    return 0;
}
