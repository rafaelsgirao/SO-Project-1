#include "fs/operations.h"
#include <assert.h>
#include <pthread.h>
#include <stdbool.h>
#include <stdint.h>
#include <stdio.h>
#include <string.h>
#include <unistd.h>

const char FILE_NAME[] = "/f1";
const char HARD_LINK[] = "/hard";
const char SYM_LINK[] = "/sym";
const char FILE_CONTENT[] = "Hello padawan!";

void verify_content(const char file[]) {
    int f = tfs_open(file, TFS_O_CREAT);
    assert(f != -1);

    char buffer[sizeof(FILE_CONTENT)];

    assert(tfs_read(f, buffer, sizeof(FILE_CONTENT)) == sizeof(FILE_CONTENT));
    assert(memcmp(buffer, FILE_CONTENT, sizeof(FILE_CONTENT)) == 0);

    assert(tfs_close(f) == 0);
}

void verify_file_inexistent(const char file[]) {
    int f = tfs_open(file, TFS_O_APPEND);
    assert(f == -1);
}

// This test asserts that multiple threads
int main() {
    assert(tfs_init(NULL) != -1);

    // creates the file
    int fhandle = tfs_open(FILE_NAME, TFS_O_CREAT);
    assert(fhandle != -1);

    assert(tfs_write(fhandle, FILE_CONTENT, sizeof(FILE_CONTENT)) ==
           sizeof(FILE_CONTENT));

    assert(tfs_close(fhandle) == 0);

    assert(tfs_link(FILE_NAME, HARD_LINK) == 0);
    assert(tfs_sym_link(FILE_NAME, SYM_LINK) == 0);

    // verifies that the content is in all files
    verify_content(FILE_NAME);
    verify_content(HARD_LINK);
    verify_content(SYM_LINK);

    // unlink all cases and verifies that that it can be opened anymore
    assert(tfs_unlink(FILE_NAME) == 0);
    verify_file_inexistent(FILE_NAME);

    assert(tfs_unlink(HARD_LINK) == 0);
    verify_file_inexistent(HARD_LINK);

    // all hard links were deleted, so the file must not exist, and the sym link
    // should point to an inexistent file
    verify_file_inexistent(SYM_LINK);

    assert(tfs_unlink(SYM_LINK) == 0);

    // destroy TécnicoFS
    assert(tfs_destroy() != -1);
    printf("Successful test.\n");
    return 0;
}