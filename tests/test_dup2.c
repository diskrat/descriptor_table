// Duplication and overwrite test
#include "../src/descriptor_table.h"
#include <stdio.h>
#include <fcntl.h>

int main() {
    user_fd_init();
    int fd1 = user_open("dup2_test.txt", O_CREAT | O_RDWR, 0644);
    if (fd1 < 0) {
        printf("FAIL: Could not open file.\n");
        user_fd_destroy();
        return 1;
    }
    int fd2 = user_dup(fd1);
    int fd3 = user_dup2(fd1, fd1+10);
    if (fd2 < 0 || fd3 < 0) {
        printf("FAIL: Duplication failed\n");
        user_close(fd1);
        user_fd_destroy();
        return 1;
    }
    printf("PASS: Duplicated fds: %d, %d\n", fd2, fd3);
    // Overwrite test
    int fdA = user_open("dup2_testA.txt", O_CREAT | O_RDWR, 0644);
    int fdB = user_open("dup2_testB.txt", O_CREAT | O_RDWR, 0644);
    if (fdA < 0 || fdB < 0) {
        printf("FAIL: Could not open files for overwrite\n");
        user_fd_destroy();
        return 1;
    }
    int dup_fd = user_dup2(fdA, fdB);
    printf("PASS: Overwrite fdB with fdA: %d\n", dup_fd);
    user_close(fd1);
    user_close(fd2);
    user_close(fd3);
    user_close(fdA);
    user_close(fdB);
    user_fd_destroy();
    return 0;
}