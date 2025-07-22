#include "../src/descriptor_table.h"
#include <stdio.h>
#include <fcntl.h>
#include <unistd.h>

int main() {
    user_fd_init();
    int fd1 = user_open("test1.txt", O_CREAT | O_RDWR, 0644);
    int fd2 = user_open("test1.txt", O_CREAT | O_RDWR, 0644);
    int fd3 = user_open("test1.txt", O_CREAT | O_RDWR, 0644);
    if (fd1 < 0 || fd2 < 0 || fd3 < 0) {
        printf("Failed to open file\n");
        return 1;
    }
    printf("Opened file with user fds: %d, %d, %d\n", fd1, fd2, fd3);
    if (!(fd1 == fd2 && fd2 == fd3)) {
        printf("Error: user_open returned different fds for same file\n");
        return 1;
    }
    
    user_close(fd1);
    user_close(fd2);
    user_close(fd3);

    user_fd_destroy();
    return 0;
}
