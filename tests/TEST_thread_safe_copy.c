#include "fs/operations.h"
#include <assert.h>
#include <pthread.h>
#include <stdbool.h>
#include <stdint.h>
#include <stdio.h>
#include <string.h>

#define THREAD_COUNT 3

void *thread_copy_from_external_file();
void *thread_read();
void *thread_write();

int main() {
    assert(tfs_init(NULL) != -1);

    // 1. Threads Copy from an External File
    pthread_t copy_pthreads[THREAD_COUNT];

    for (int i = 0; i < THREAD_COUNT; i++) {
        assert(pthread_create(&copy_pthreads[i], NULL,
                              thread_copy_from_external_file, NULL) == 0);
    }

    for (int i = 0; i < THREAD_COUNT; i++) {
        assert(pthread_join(copy_pthreads[i], NULL) == 0);
    }

    // 2. Threads read the files
    pthread_t read_pthreads[THREAD_COUNT];

    for (int i = 0; i < THREAD_COUNT; i++) {
        assert(pthread_create(&read_pthreads[i], NULL, thread_read, NULL) == 0);
    }
    for (int i = 0; i < THREAD_COUNT; i++) {
        assert(pthread_join(read_pthreads[i], NULL) == 0);
    }

    // 3. Threads write to an tfs file
    pthread_t write_pthreads[THREAD_COUNT];

    for (int i = 0; i < THREAD_COUNT; i++) {
        assert(pthread_create(&write_pthreads[i], NULL, thread_write, NULL) ==
               0);
    }

    for (int i = 0; i < THREAD_COUNT; i++) {
        assert(pthread_join(write_pthreads[i], NULL) == 0);
    }

    // destroy TécnicoFS
    assert(tfs_destroy() != -1);
    printf("Successful test.\n");
    return 0;
}

void *thread_copy_from_external_file() {
    char *path_copied_file = "/file";
    char *path_src = "tests/file_to_copy.txt";

    int f;

    f = tfs_copy_from_external_fs(path_src, path_copied_file);
    assert(f != -1);

    return 0;
}

void *thread_write() {
    char *str_ext_file = "BBB!";
    char *path_copied_file = "/file";
    char buffer[40];

    int f;
    ssize_t r;

    f = tfs_open(path_copied_file, TFS_O_CREAT);
    assert(f != -1);

    r = tfs_read(f, buffer, sizeof(buffer) - 1);
    assert(r == strlen(str_ext_file));
    assert(!memcmp(buffer, str_ext_file, strlen(str_ext_file)));

    assert(tfs_close(f) == 0);

    return 0;
}

void *thread_read() {
    char *str_ext_file = "BBB!";
    char *path_copied_file = "/file";
    char buffer[40];

    int f;
    ssize_t r;

    f = tfs_open(path_copied_file, TFS_O_CREAT);
    assert(f != -1);

    // Contents should be overwriten, not appended
    r = tfs_read(f, buffer, sizeof(buffer) - 1);
    assert(r == strlen(str_ext_file));
    assert(!memcmp(buffer, str_ext_file, strlen(str_ext_file)));

    assert(tfs_close(f) == 0);

    return 0;
}
