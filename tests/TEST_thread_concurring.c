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

void *thread_read();
void *thread_write();

uint8_t const MESSAGE[] = "Lorem ipsum dolor sit amet.";

// This test asserts that multiple threads
int main() {
    assert(tfs_init(NULL) != -1);

    pthread_t t1, t2, t3;

    // first thread will read the file
    assert(pthread_create(&t1, NULL, thread_read, NULL) == 0);

    // second thread will write the file
    assert(pthread_create(&t2, NULL, thread_write, NULL) == 0);

    // third thread will read the file
    assert(pthread_create(&t3, NULL, thread_read, NULL) == 0);

    // waits for all threads to end
    assert(pthread_join(t1, NULL) == 0);
    assert(pthread_join(t2, NULL) == 0);
    assert(pthread_join(t3, NULL) == 0);

    // destroy TécnicoFS
    assert(tfs_destroy() != -1);
    printf("Successful test.\n");
    return 0;
}

// reads the message from the file
void *thread_read() {
    sleep(1); // waits for 1 sec (while the t2 writes in the file)

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