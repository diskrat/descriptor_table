#include "../src/descriptor_table.h"
#include <stdio.h>

int main() {
    user_fd_init();

    // Test invalid file descriptor for close
    if (user_close(-1) == -1) {
        printf("Handled invalid file descriptor for close.\n");
    }

    // Test invalid file descriptor for read
    char buf[10];
    if (user_read(-1, buf, sizeof(buf)) == -1) {
        printf("Handled invalid file descriptor for read.\n");
    }

    // Test invalid file descriptor for write
    if (user_write(-1, "data", 4) == -1) {
        printf("Handled invalid file descriptor for write.\n");
    }

    user_fd_destroy();
    return 0;
}
