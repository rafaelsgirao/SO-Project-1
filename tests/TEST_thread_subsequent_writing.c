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
#define THREAD_COUNT 3
#define DEBUG 1

void *first_read();
void *last_read();
void *thread_write();

uint8_t const MESSAGE[] = "A";
uint8_t const FINAL_MESSAGE[] = "AAA";

// This test asserts that multiple threads
int main() {
    assert(tfs_init(NULL) != -1);

    pthread_t writers[THREAD_COUNT];
    pthread_t first_reader, last_reader;

    assert(pthread_create(&first_reader, NULL, first_read, NULL) == 0);
    assert(pthread_join(first_reader, NULL) == 0);

    for (int i = 0; i < THREAD_COUNT; i++) {
        assert(pthread_create(&writers[i], NULL, thread_write, NULL) == 0);
    }

    for (int i = 0; i < THREAD_COUNT; i++) {
        assert(pthread_join(writers[i], NULL) == 0);
    }

    assert(pthread_create(&last_reader, NULL, last_read, NULL) == 0);
    assert(pthread_join(last_reader, NULL) == 0);

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
    uint8_t buffer[sizeof(FINAL_MESSAGE)];

    int fhandle = tfs_open(FILE_SUBJECT, TFS_O_CREAT);
    assert(fhandle != -1);

    ssize_t len = tfs_read(fhandle, buffer, sizeof(buffer));
    printf("buffer: %s\n", buffer);
    assert(len == sizeof(buffer));

    if (DEBUG)
        printf("\nBUFFER: %s\n", buffer);

    assert(memcmp(buffer, FINAL_MESSAGE, sizeof(buffer)) == 0);
    assert(tfs_close(fhandle) == 0);

    return NULL;
}

// writes the message in the file
void *thread_write() {

    int fhandle = tfs_open(FILE_SUBJECT, TFS_O_APPEND);
    assert(fhandle != -1);

    uint8_t buffer[sizeof(MESSAGE)];

    assert(tfs_write(fhandle, MESSAGE, sizeof(MESSAGE)) == sizeof(MESSAGE));

    assert(tfs_close(fhandle) == 0);

    fhandle = tfs_open(FILE_SUBJECT, TFS_O_CREAT);

    assert(tfs_read(fhandle, buffer, sizeof(buffer)) == sizeof(buffer));

    if (DEBUG)
        printf("BUFFER-WRITE: %s\n", buffer);

    assert(tfs_close(fhandle) == 0);

    return NULL;
}