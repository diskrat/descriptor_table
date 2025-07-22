#include "../src/descriptor_table.h"
#include <stdio.h>
#include <string.h>
#include <fcntl.h>

int main() {
    user_fd_init();

    int fd = user_open("large_file.txt", O_CREAT | O_WRONLY, 0644);
    if (fd == -1) {
        printf("Failed to open large_file.txt\n");
        return 1;
    }

    // Write a large amount of data
    const char* data = "Large data block\n";
    for (int i = 0; i < 100000; i++) {
        user_write(fd, data, strlen(data));
    }

    user_close(fd);
    user_fd_destroy();
    return 0;
}
