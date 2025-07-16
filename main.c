#include <pthread.h>
#include <unistd.h>
#include <fcntl.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h> 
#include <stdint.h> // For UINT32_MAX
#include <stdbool.h> // For boolean type

#define MAX_USER_FDS 256
#define INITIAL_CAPACITY 2

typedef struct {
    int kernel_fd;
    int ref_count;
    char* path;
    bool is_permanent; // New field to track if the file is permanent
} UserFD;

typedef struct {
    int size; 
    int capacity;
    int lowest_id; 
    pthread_mutex_t fd_lock;
    UserFD *table;
}FD_Table;

FD_Table descriptor_table;

int check_fd_existence(const char* path) {
    for (int i = 0; i < descriptor_table.size; i++) {
        if (descriptor_table.table[i].path != NULL && strcmp(descriptor_table.table[i].path, path) == 0) {
            if (descriptor_table.table[i].ref_count < UINT32_MAX) {
                descriptor_table.table[i].ref_count++;
            } else {
                descriptor_table.table[i].is_permanent = true;
            }
            return i;
        }
    }
    return -1;
}

void double_capacity() {
    if (descriptor_table.capacity < MAX_USER_FDS) {
        int new_capacity = descriptor_table.capacity * 2;
        UserFD* new_table = (UserFD*)realloc(descriptor_table.table, new_capacity * sizeof(UserFD));
        if (new_table == NULL) {
            perror("Failed to reallocate memory");
            exit(EXIT_FAILURE);
        }
        descriptor_table.table = new_table;
        descriptor_table.capacity = new_capacity;
    }
}

int find_lowest_available() {
    for (int i = descriptor_table.lowest_id; i < descriptor_table.capacity; ++i) {
        if (descriptor_table.table[i].kernel_fd == -1) {
            descriptor_table.lowest_id = i; 
            return i;
        }
    }
    return -1; 
}

void user_fd_system_init() {
    descriptor_table.size = 0;
    descriptor_table.lowest_id = 0;
    pthread_mutex_init(&descriptor_table.fd_lock,NULL);
    descriptor_table.table = (UserFD*)malloc(INITIAL_CAPACITY*sizeof(UserFD));
    if (descriptor_table.table == NULL) {
        perror("Failed to allocate memory");
        exit(EXIT_FAILURE);
    }
    descriptor_table.capacity = INITIAL_CAPACITY;
}

void user_fd_system_destroy() {
    if (descriptor_table.table != NULL) {
        for (int i = 0; i < descriptor_table.capacity; i++) {
            if (descriptor_table.table[i].path != NULL) {
                free(descriptor_table.table[i].path);
                descriptor_table.table[i].path = NULL;
            }
        }
        free(descriptor_table.table);
        descriptor_table.table = NULL;
    }
    descriptor_table.size = 0;
    descriptor_table.capacity = 0;
}

int user_open(const char* path, int flags, mode_t mode) {
    pthread_mutex_lock(&descriptor_table.fd_lock);
    int fd_n = check_fd_existence(path);
    if (fd_n != -1) {
        pthread_mutex_unlock(&descriptor_table.fd_lock);
        return fd_n;
    }
    pthread_mutex_unlock(&descriptor_table.fd_lock);
    
    int kernel_fd = open(path, flags, mode);
    if (kernel_fd == -1) {
        perror("Failed to open file");
        return -1;
    }

    pthread_mutex_lock(&descriptor_table.fd_lock);
    int index = find_lowest_available();
    if (index == -1) {
        if (descriptor_table.size >= descriptor_table.capacity) {
            double_capacity();
        }
        index = descriptor_table.size++;
    }

    descriptor_table.table[index].kernel_fd = kernel_fd;
    descriptor_table.table[index].ref_count = 1;
    descriptor_table.table[index].path = strdup(path);
    descriptor_table.table[index].is_permanent = false;
    pthread_mutex_unlock(&descriptor_table.fd_lock);
    return index;
}


int user_close(int user_fd) {
    if (user_fd < 0 || user_fd >= descriptor_table.capacity) {
        fprintf(stderr, "Invalid user_fd: %d\n", user_fd);
        return -1; 
    }

    pthread_mutex_lock(&descriptor_table.fd_lock);
    if (descriptor_table.table[user_fd].kernel_fd == -1) {
        pthread_mutex_unlock(&descriptor_table.fd_lock);
        fprintf(stderr, "File descriptor not in use: %d\n", user_fd);
        return -1;
    }

    if (descriptor_table.table[user_fd].is_permanent) {
        // File is permanent, do not decrement ref_count or close it
        pthread_mutex_unlock(&descriptor_table.fd_lock);
        return 0;
    }

    descriptor_table.table[user_fd].ref_count--;
    if (descriptor_table.table[user_fd].ref_count == 0) {
        close(descriptor_table.table[user_fd].kernel_fd);
        descriptor_table.table[user_fd].kernel_fd = -1;
        descriptor_table.table[user_fd].ref_count = 0;
        free(descriptor_table.table[user_fd].path);
        descriptor_table.table[user_fd].path = NULL;

        if (user_fd < descriptor_table.lowest_id) {
            descriptor_table.lowest_id = user_fd; 
        }
    }
    pthread_mutex_unlock(&descriptor_table.fd_lock);
    return 0; 
}


ssize_t user_read(int user_fd, void* buf, size_t count) {
    if (user_fd < 0 || user_fd >= descriptor_table.capacity) {
        return -1; 
    }

    pthread_mutex_lock(&descriptor_table.fd_lock);
    if (descriptor_table.table[user_fd].kernel_fd == -1) {
        pthread_mutex_unlock(&descriptor_table.fd_lock);
        return -1; 
    }
    int kernel_fd = descriptor_table.table[user_fd].kernel_fd;
    pthread_mutex_unlock(&descriptor_table.fd_lock);
    return read(kernel_fd, buf, count);
}


ssize_t user_write(int user_fd, const void* buf, size_t count) {
    if (user_fd < 0 || user_fd >= descriptor_table.capacity) {
        fprintf(stderr, "Invalid user_fd: %d\n", user_fd);
        return -1; 
    }

    if (buf == NULL || count == 0) {
        fprintf(stderr, "Invalid buffer or count\n");
        return -1;
    }

    pthread_mutex_lock(&descriptor_table.fd_lock);
    if (descriptor_table.table[user_fd].kernel_fd == -1) {
        pthread_mutex_unlock(&descriptor_table.fd_lock);
        fprintf(stderr, "File descriptor not in use: %d\n", user_fd);
        return -1;
    }
    int kernel_fd = descriptor_table.table[user_fd].kernel_fd;
    pthread_mutex_unlock(&descriptor_table.fd_lock);

    ssize_t result = write(kernel_fd, buf, count);
    if (result == -1) {
        perror("write failed");
    }
    return result;
}

int user_dup(int old_user_fd) {
    if (old_user_fd < 0 || old_user_fd >= descriptor_table.capacity ||
        descriptor_table.table[old_user_fd].kernel_fd == -1) {
        return -1;
    }

    int new_user_fd = find_lowest_available();
    if (new_user_fd == -1) {
        return -1;
    }

    descriptor_table.table[new_user_fd] = descriptor_table.table[old_user_fd];
    descriptor_table.table[new_user_fd].ref_count++;
    return new_user_fd;
}

int user_dup2(int old_user_fd, int new_user_fd) {
    if (old_user_fd < 0 || old_user_fd >= descriptor_table.capacity ||
        new_user_fd < 0 || new_user_fd >= MAX_USER_FDS ||
        descriptor_table.table[old_user_fd].kernel_fd == -1) {
        return -1;
    }

    if (old_user_fd == new_user_fd) {
        return new_user_fd;
    }

    if (new_user_fd < descriptor_table.capacity &&
        descriptor_table.table[new_user_fd].kernel_fd != -1) {
        user_close(new_user_fd);
    }

    if (new_user_fd >= descriptor_table.capacity) {
        while (descriptor_table.capacity <= new_user_fd) {
            double_capacity();
        }
    }

    descriptor_table.table[new_user_fd] = descriptor_table.table[old_user_fd];
    descriptor_table.table[new_user_fd].ref_count++;
    return new_user_fd;
}

int main() {
    user_fd_system_init();

    int fd1 = user_open("test1.txt", O_CREAT | O_WRONLY, 0644);
    int fd2 = user_open("test2.txt", O_CREAT | O_WRONLY, 0644);
    int fd3 = user_open("test1.txt", O_CREAT | O_WRONLY, 0644); 
    printf("fd1: %d, fd2: %d, fd3: %d\n", fd1, fd2, fd3);

    
    const char* msg1 = "Hello, test1!";
    const char* msg2 = "Hello, test2!";
    user_write(fd1, msg1, strlen(msg1));
    user_write(fd2, msg2, strlen(msg2));
    printf("written\n");

    
    user_close(fd1); 
    user_close(fd3); 
    printf("closed\n");

    char buf[50];
    int fd4 = user_open("test1.txt", O_RDONLY, 0);
    ssize_t bytes_read = user_read(fd4, buf, sizeof(buf) - 1);
    if (bytes_read > 0) {
        buf[bytes_read] = '\0';
        printf("Read from test1.txt: %s\n", buf);
    }
    printf("read\n");
    user_close(fd4);
    user_close(fd2);
    printf("closed\n");
    

    int dup_fd1 = user_dup(fd1);
    if (dup_fd1 != -1) {
        printf("dup_fd1: %d (duplicado de fd1)\n", dup_fd1);
        user_write(dup_fd1, " Duplicated", 10);
    } else {
        printf("Falha ao duplicar fd1\n");
    }

    int dup2_fd = user_dup2(fd2, 10);
    if (dup2_fd != -1) {
        printf("dup2_fd: %d (fd2 redirecionado para 10)\n", dup2_fd);
        user_write(dup2_fd, " Redirected", 11);
    } else {
        printf("Falha ao redirecionar fd2 para 10\n");
    }


    int invalid_fd = user_close(999); 
    printf("Closing invalid fd: %d\n", invalid_fd);

    // Saturation test for ref_count
    printf("Starting saturation test for ref_count...\n");
    int fd_saturation = user_open("saturation_test.txt", O_CREAT | O_WRONLY, 0644);
    if (fd_saturation == -1) {
        printf("Failed to open saturation_test.txt\n");
    } else {
        for (uint32_t i = 0; i < UINT32_MAX; i++) {
            if (user_open("saturation_test.txt", O_WRONLY, 0) == -1) {
                printf("Failed to increment ref_count at iteration %u\n", i);
                break;
            }
        }

        // Check if the file is marked as permanent
        if (descriptor_table.table[fd_saturation].is_permanent) {
            printf("File saturation_test.txt is now permanent.\n");
        } else {
            printf("File saturation_test.txt is not marked as permanent.\n");
        }

        user_close(fd_saturation);
    }

    user_fd_system_destroy();
    return 0;
}