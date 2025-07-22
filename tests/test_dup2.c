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

    // Create 5 duplicates of fd1 using user_dup2
    int dup2_fds[5];
    for (int i = 0; i < 5; i++) {
        dup2_fds[i] = user_dup2(fd1, fd1 + 10 + i); // Use new_user_fd values far from fd1
        if (dup2_fds[i] < 0) {
            printf("Failed to duplicate file descriptor with dup2\n");
            return 1;
        }
        printf("Duplicated file descriptor with dup2: %d\n", dup2_fds[i]);
    }

    // Overwrite test: open two different files and overwrite one fd with another
    int fdA = user_open("test1.txt", O_CREAT | O_WRONLY, 0644);
    int fdB = user_open("test2.txt", O_CREAT | O_WRONLY, 0644);
    if (fdA < 0 || fdB < 0) {
        printf("Failed to open files for overwrite test\n");
        return 1;
    }
    printf("[Overwrite test] Opened file descriptors: %d (test1.txt), %d (test2.txt)\n", fdA, fdB);
    int dup_fd = user_dup2(fdA, fdB);
    printf("[Overwrite test] After dup2(fdA, fdB): fdB now points to fd: %d\n", dup_fd);
    if (dup_fd != fdB) {
        printf("[Overwrite test] dup2 did not return the expected fdB value!\n");
        return 1;
    }
    printf("[Overwrite test] fdA and fdB now reference the same descriptor.\n");

    // Close all but one file descriptor
    user_close(fd1);
    user_close(fd2);
    user_close(fd3);
    for (int i = 0; i < 4; i++) {
        user_close(dup2_fds[i]);
    }
    user_close(fdA);
    user_close(fdB);

    // Keep the last duplicate open
    printf("Kept file descriptor open: %d\n", dup2_fds[4]);

    user_fd_destroy();
    return 0;
}
