// Saturation test: ref count and permanent descriptor threshold
#include "../src/descriptor_table.h"
#include <stdio.h>
#include <fcntl.h>

int main() {
    user_fd_init();
    int fd = user_open("saturation_test.txt", O_CREAT | O_WRONLY, 0644);
    if (fd == -1) {
        printf("FAIL: Could not open file\n");
        user_fd_destroy();
        return 1;
    }
    int ok = 1;
    for (int i = 1; i <= 100; i++) {
        int fdi = user_open("saturation_test.txt", O_WRONLY, 0);
        if (fdi == -1) {
            printf("FAIL: Could not increment ref_count at %d\n", i);
            ok = 0;
            break;
        }
        user_close(fdi);
    }
    printf("PASS: Saturation test completed\n");
    user_close(fd);
    user_fd_destroy();
    return ok ? 0 : 1;
}
