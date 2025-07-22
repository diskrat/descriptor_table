#include "../src/descriptor_table.h"
#include <stdio.h>
#include <stdint.h>
#include <fcntl.h>


int main() {
    user_fd_init();

    int fd = user_open("saturation_test.txt", O_CREAT | O_WRONLY, 0644);
    if (fd == -1) {
        printf("Failed to open saturation_test.txt\n");
        return 1;
    }

    for (uint32_t i = 0; i < UINT32_MAX; i++) {
        if (i == PERMANENT_THRESHOLD) {
            printf("Reached permanent threshold at iteration %u. Marking file as permanent.\n", i);
            break;
        }
        if (user_open("saturation_test.txt", O_WRONLY, 0) == -1) {
            printf("Failed to increment ref_count at iteration %u\n", i);
            break;
        }
    }

    // File is now permanent, skipping user_close
    user_fd_destroy();
    return 0;
}
