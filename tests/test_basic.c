#include "../src/descriptor_table.h"
#include <stdio.h>
#include <string.h>
#include <fcntl.h>
#include <unistd.h>

int main() {
    user_fd_init();

    int fd1 = user_open("test1.txt", O_CREAT | O_WRONLY, 0644);
    printf("Opened file descriptor: %d\n", fd1);

    const char* msg = "Hello, test1!";
    user_write(fd1, msg, strlen(msg));
    user_close(fd1);

    // Read/write test: reopen for reading and verify contents
    int fd2 = user_open("test1.txt", O_RDONLY, 0644);
    if (fd2 < 0) {
        printf("Failed to reopen file for reading\n");
        user_fd_destroy();
        return 1;
    }
    char buf[64] = {0};
    ssize_t n = user_read(fd2, buf, sizeof(buf)-1);
    if (n < 0) {
        printf("Failed to read from file\n");
        user_close(fd2);
        user_fd_destroy();
        return 1;
    }
    printf("Read from file: '%s'\n", buf);
    if (strncmp(buf, msg, strlen(msg)) == 0) {
        printf("Read/write test passed.\n");
    } else {
        printf("Read/write test failed!\n");
    }
    user_close(fd2);


    user_fd_destroy();
    return 0;
}
