#include "fs/operations.h"
#include <assert.h>
#include <pthread.h>
#include <stdbool.h>
#include <stdint.h>
#include <stdio.h>
#include <string.h>
#include <unistd.h>

#define FILE_SUBJECT "/file"
#define BUFFER_SIZE 500
#define THREAD_COUNT 5

void *first_read();
void *last_read();
void *thread_write();

uint8_t const MESSAGE[] = "Lorem ipsum dolor sit amet.";

// This test asserts that multiple threads
int main() {
    assert(tfs_init(NULL) != -1);

    pthread_t writers[THREAD_COUNT];
    pthread_t reader;

    assert(pthread_create(&reader, NULL, first_read, NULL) == 0);
    assert(pthread_join(reader, NULL) == 0);

    for (int i = 0; i < THREAD_COUNT; i++) {
        assert(pthread_create(&writers[i], NULL, thread_write, NULL) == 0);
    }

    for (int i = 0; i < THREAD_COUNT; i++) {
        assert(pthread_join(writers[i], NULL) == 0);
    }

    // second thread will read the file
    assert(pthread_create(&reader, NULL, last_read, NULL) == 0);
    assert(pthread_join(reader, NULL) == 0);

    // destroy TécnicoFS
    assert(tfs_destroy() != -1);
    printf("Successful test.\n");
    return 0;
}

void *first_read() {
    uint8_t buffer[sizeof(MESSAGE)];

    int fhandle = tfs_open(FILE_SUBJECT, TFS_O_CREAT);
    assert(fhandle != -1);

    // file should be empty
    assert(tfs_read(fhandle, buffer, sizeof(buffer)) == 0);
    assert(tfs_close(fhandle) == 0);

    return NULL;
}

// reads the message from the file
void *last_read() {
    uint8_t buffer[sizeof(MESSAGE)];

    int fhandle = tfs_open(FILE_SUBJECT, TFS_O_CREAT);
    assert(fhandle != -1);

    assert(tfs_read(fhandle, buffer, sizeof(buffer)) == sizeof(buffer));
    assert(memcmp(buffer, MESSAGE, sizeof(buffer)) == 0);
    assert(tfs_close(fhandle) == 0);

    return NULL;
}

// writes the message in the file
void *thread_write() {
    int fhandle = tfs_open(FILE_SUBJECT, TFS_O_CREAT);
    assert(fhandle != -1);

    assert(tfs_write(fhandle, MESSAGE, sizeof(MESSAGE)) == sizeof(MESSAGE));
    assert(tfs_close(fhandle) == 0);

    return NULL;
}