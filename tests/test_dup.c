#include "../src/descriptor_table.h"
#include <stdio.h>
#include <fcntl.h>

int main() {
    user_fd_init();

    // Open the same file 3 times
    int fd1 = user_open("test1.txt", O_CREAT | O_WRONLY, 0644);
    int fd2 = user_open("test1.txt", O_CREAT | O_WRONLY, 0644);
    int fd3 = user_open("test1.txt", O_CREAT | O_WRONLY, 0644);

    if (fd1 < 0 || fd2 < 0 || fd3 < 0) {
        printf("Failed to open file\n");
        return 1;
    }

    printf("Opened file descriptors: %d, %d, %d\n", fd1, fd2, fd3);

    // Create 5 duplicates of fd1
    int dup_fds[5];
    for (int i = 0; i < 5; i++) {
        dup_fds[i] = user_dup(fd1);
        if (dup_fds[i] < 0) {
            printf("Failed to duplicate file descriptor\n");
            return 1;
        }
        printf("Duplicated file descriptor: %d\n", dup_fds[i]);
    }

    // Close all but one file descriptor
    user_close(fd1);
    user_close(fd2);
    user_close(fd3);
    for (int i = 0; i < 4; i++) {
        user_close(dup_fds[i]);
    }

    // Keep the last duplicate open
    printf("Kept file descriptor open: %d\n", dup_fds[4]);

    user_fd_destroy();
    return 0;
}
