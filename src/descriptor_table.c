#include "descriptor_table.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <fcntl.h>

D_Tabela dt;

void user_fd_init() 
{
    dt.size = 0;
    dt.lowest_id = 0;
    dt.capacity = INITIAL_CAPACITY;
    pthread_rwlock_init(&dt.fd_lock, NULL);
    dt.table = (Descritor**)malloc(INITIAL_CAPACITY * sizeof(Descritor*));
    if (dt.table == NULL) {
        perror("Failed to allocate memory");
        exit(EXIT_FAILURE);
    }
    for (int i = 0; i < dt.capacity; i++) {
        dt.table[i] = NULL;
    }
}

void user_fd_destroy() 
{
    pthread_rwlock_wrlock(&dt.fd_lock);
    for (int i = 0; i < dt.capacity; i++) {
        if (dt.table[i]) {
            pthread_rwlock_wrlock(&dt.table[i]->lock);
            if (dt.table[i]->kernel_fd != -1) {
                close(dt.table[i]->kernel_fd);
                dt.table[i]->kernel_fd = -1;
                dt.table[i]->ref_count = 0;
                free(dt.table[i]->path);
                dt.table[i]->path = NULL;
                dt.table[i]->is_permanent = false;
            }
            pthread_rwlock_unlock(&dt.table[i]->lock);
            pthread_rwlock_destroy(&dt.table[i]->lock);
            free(dt.table[i]);
            dt.table[i] = NULL;
        }
    }
    free(dt.table);
    dt.table = NULL;
    dt.size = 0;
    dt.capacity = 0;
    dt.lowest_id = 0;
    pthread_rwlock_unlock(&dt.fd_lock);
    pthread_rwlock_destroy(&dt.fd_lock);
}

void double_capacity() 
{
    if (dt.capacity < MAX_USER_FDS) {
        int new_capacity = dt.capacity * 2;
        if (new_capacity > MAX_USER_FDS) {
            new_capacity = MAX_USER_FDS;
        }
        Descritor** new_table = (Descritor**)realloc(dt.table, new_capacity * sizeof(Descritor*));
        if (new_table == NULL) {
            perror("Failed to reallocate memory");
            exit(EXIT_FAILURE);
        }
        for (int i = dt.capacity; i < new_capacity; i++) {
            new_table[i] = NULL;
        }
        dt.table = new_table;
        dt.capacity = new_capacity;
        if (dt.lowest_id >= dt.capacity) {
            dt.lowest_id = dt.capacity - 1;
        }
    }
}

int user_open(char* path, int flags, mode_t mode) {
    pthread_rwlock_wrlock(&dt.fd_lock);
    int kernel_fd = open(path, flags, mode);
    if (kernel_fd == -1) {
        perror("Failed to open file");
        pthread_rwlock_unlock(&dt.fd_lock);
        return -1;
    }
    int index = dt.lowest_id;
    dt.size++;
    if (index >= dt.capacity) {
        double_capacity();
    }
    for (int i = index + 1; i < dt.capacity; i++) {
        if (!dt.table[i]) {
            dt.lowest_id = i;
            break;
        }
    }
    
    bool file_found = false;
    for (int i = 0; i < dt.capacity; i++) {
        if (dt.table[i] && dt.table[i]->path != NULL && strcmp(dt.table[i]->path, path) == 0) {
            pthread_rwlock_wrlock(&dt.table[i]->lock);
            if (!dt.table[i]->is_permanent) {
                dt.table[i]->ref_count++;
                if (dt.table[i]->ref_count >= PERMANENT_THRESHOLD) {
                    dt.table[i]->is_permanent = true;
                }
            }
            pthread_rwlock_unlock(&dt.table[i]->lock);
            file_found = true;
            dt.table[index] = dt.table[i];
            break;
        }
    }
    if(!file_found){ 
        Descritor* desc = (Descritor*)malloc(sizeof(Descritor));
        if (!desc) {
            perror("Failed to allocate descriptor");
            pthread_rwlock_unlock(&dt.fd_lock);
            return -1;
        } 
        desc->ref_count = 1;
        desc->path = strdup(path);
        desc->kernel_fd = kernel_fd;
        desc->is_permanent = false;
        pthread_rwlock_init(&desc->lock, NULL);
        dt.table[index] = desc;   
    }
    pthread_rwlock_unlock(&dt.fd_lock);
    return index;
}

int user_close(int user_fd) 
{
    pthread_rwlock_rdlock(&dt.fd_lock);
    if (user_fd < 0 || user_fd >= dt.capacity || dt.table[user_fd] == NULL) {
        pthread_rwlock_unlock(&dt.fd_lock);
        return -1;
    }
    Descritor* desc = dt.table[user_fd];
    pthread_rwlock_wrlock(&desc->lock);
    if (desc->kernel_fd == -1) {
        pthread_rwlock_unlock(&desc->lock);
        pthread_rwlock_unlock(&dt.fd_lock);
        return -1;
    }
    desc->ref_count--;
    dt.table[user_fd] = NULL;
    dt.size--;
    if (user_fd < dt.capacity && user_fd < dt.lowest_id) {
        dt.lowest_id = user_fd;
    }
    if(!desc->is_permanent && desc->ref_count==0){
        desc->kernel_fd = -1;
        free(desc->path);
        desc->path = NULL;
        close(desc->kernel_fd);
        pthread_rwlock_unlock(&desc->lock);
        pthread_rwlock_destroy(&desc->lock);
        free(desc);
    } else {
        pthread_rwlock_unlock(&desc->lock);
    }
    pthread_rwlock_unlock(&dt.fd_lock);
    return 0;
}

ssize_t user_read(int user_fd, void* buf, size_t count) 
{
    if (user_fd < 0 || user_fd >= dt.capacity || !dt.table[user_fd]) {
        return -1;
    }
    pthread_rwlock_rdlock(&dt.table[user_fd]->lock);
    if (dt.table[user_fd]->kernel_fd == -1) {
        pthread_rwlock_unlock(&dt.table[user_fd]->lock);
        return -1;
    }
    int kernel_fd = dt.table[user_fd]->kernel_fd;
    ssize_t result = read(kernel_fd, buf, count);
    pthread_rwlock_unlock(&dt.table[user_fd]->lock);
    return result;
}

ssize_t user_write(int user_fd, const void* buf, size_t count) 
{
    if (user_fd < 0 || user_fd >= dt.capacity || !dt.table[user_fd]) {
        return -1;
    }
    if (buf == NULL || count == 0) {
        return -1;
    }
    pthread_rwlock_wrlock(&dt.table[user_fd]->lock);
    if (dt.table[user_fd]->kernel_fd == -1) {
        pthread_rwlock_unlock(&dt.table[user_fd]->lock);
        return -1;
    }
    int kernel_fd = dt.table[user_fd]->kernel_fd;
    ssize_t result = write(kernel_fd, buf, count);
    pthread_rwlock_unlock(&dt.table[user_fd]->lock);
    return result;
}

int user_dup(int old_user_fd) 
{
    pthread_rwlock_wrlock(&dt.fd_lock);
    pthread_rwlock_wrlock(&dt.table[old_user_fd]->lock);
    if (old_user_fd < 0 || old_user_fd >= dt.capacity || dt.table[old_user_fd]->kernel_fd == -1) {
        pthread_rwlock_unlock(&dt.table[old_user_fd]->lock);
        pthread_rwlock_unlock(&dt.fd_lock);
        return -1;
    }
    if (dt.size >= dt.capacity) {
        double_capacity();
    }
    int new_user_fd = dt.lowest_id;
    dt.table[old_user_fd]->ref_count++;
    dt.table[new_user_fd] = dt.table[old_user_fd];
    dt.size++;
    for (int i = 0; i < dt.capacity; i++) {
        if (!dt.table[i]) {
            dt.lowest_id = i;
            break;
        }
    }
    pthread_rwlock_unlock(&dt.table[old_user_fd]->lock);
    pthread_rwlock_unlock(&dt.fd_lock);
    return new_user_fd;
}

int user_dup2(int old_user_fd, int new_user_fd) 
{
    pthread_rwlock_rdlock(&dt.fd_lock);
    pthread_rwlock_wrlock(&dt.table[old_user_fd]->lock);
    if (old_user_fd < 0 || old_user_fd >= MAX_USER_FDS ||
        new_user_fd < 0 || new_user_fd >= MAX_USER_FDS ||
        dt.table[old_user_fd]->kernel_fd == -1) {
        pthread_rwlock_unlock(&dt.table[old_user_fd]->lock);
        pthread_rwlock_unlock(&dt.fd_lock);
        return -1;
    }
    if (old_user_fd == new_user_fd) {
        pthread_rwlock_unlock(&dt.table[old_user_fd]->lock);
        pthread_rwlock_unlock(&dt.fd_lock);
        return new_user_fd;
    }
    if (new_user_fd >= dt.capacity) {
        pthread_rwlock_unlock(&dt.table[old_user_fd]->lock);
        pthread_rwlock_unlock(&dt.fd_lock);
        while (dt.capacity <= new_user_fd) {
            double_capacity();
        }
        pthread_rwlock_rdlock(&dt.fd_lock);
        pthread_rwlock_wrlock(&dt.table[old_user_fd]->lock);
    }
    int need_close = 0;
    if (new_user_fd < dt.capacity && dt.table[new_user_fd]) {
        need_close = 1;
    }
    if (need_close) {
        pthread_rwlock_unlock(&dt.table[old_user_fd]->lock);
        pthread_rwlock_unlock(&dt.fd_lock);
        user_close(new_user_fd);
        pthread_rwlock_rdlock(&dt.fd_lock);
        pthread_rwlock_wrlock(&dt.table[old_user_fd]->lock);
    }
    if (new_user_fd >= dt.capacity) {
        int old_capacity = dt.capacity;
        while (dt.capacity <= new_user_fd) {
            double_capacity();
        }
        for (int i = old_capacity; i < dt.capacity; i++) {
            dt.table[i] = NULL;
        }
    }
    dt.table[old_user_fd]->ref_count++;
    if (dt.table[new_user_fd] == NULL) {
        dt.size++;
    }
    dt.table[new_user_fd] = dt.table[old_user_fd];
    for (int i = 0; i < dt.capacity; i++) {
        if (!dt.table[i]) {
            dt.lowest_id = i;
            break;
        }
    }
    pthread_rwlock_unlock(&dt.table[old_user_fd]->lock);
    pthread_rwlock_unlock(&dt.fd_lock);
    return new_user_fd;
}   