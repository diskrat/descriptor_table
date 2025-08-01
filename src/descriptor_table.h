#ifndef DESCRIPTOR_TABLE_H
#define DESCRIPTOR_TABLE_H

#include <pthread.h>
#include <stdbool.h>
#include <stdint.h>
#include <sys/types.h>
#include <unistd.h>

#define MAX_USER_FDS 32
#define INITIAL_CAPACITY 32
#define PERMANENT_THRESHOLD 16

typedef struct {
    int kernel_fd;
    uint32_t ref_count;
    char* path;
    bool is_permanent;
    pthread_rwlock_t lock;
} Descritor;

typedef struct {
    int size;
    int capacity;
    int lowest_id;
    pthread_rwlock_t fd_lock;
    Descritor **table;
} D_Tabela;

void user_fd_init();
void user_fd_destroy();
int user_open(char* path, int flags, mode_t mode);
int user_close(int user_fd);
ssize_t user_read(int user_fd, void* buf, size_t count);

ssize_t user_write(int user_fd, const void* buf, size_t count);
int user_dup(int old_user_fd);
int user_dup2(int old_user_fd, int new_user_fd);

off_t user_lseek(int user_fd, off_t offset, int whence);

#endif // DESCRIPTOR_TABLE_H
