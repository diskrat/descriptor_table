// Error handling test: invalid fd operations
#include "../src/descriptor_table.h"
#include <stdio.h>

int main() {
    user_fd_init();
    char buf[8];
    int ok = 1;
    if (user_close(-1) == -1) printf("PASS: Invalid close\n"); else ok = 0;
    if (user_read(-1, buf, sizeof(buf)) == -1) printf("PASS: Invalid read\n"); else ok = 0;
    if (user_write(-1, "x", 1) == -1) printf("PASS: Invalid write\n"); else ok = 0;
    user_fd_destroy();
    return ok ? 0 : 1;
}
