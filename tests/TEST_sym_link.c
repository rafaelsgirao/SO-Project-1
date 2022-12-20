#include "fs/operations.h"
#include <assert.h>
#include <pthread.h>
#include <stdbool.h>
#include <stdint.h>
#include <stdio.h>
#include <string.h>
#include <unistd.h>

const char FILE_NAME[] = "/file";
const char FILE_CONTENT[] = "Hello!";
const char SYM_LINK_1[] = "/sym_1";
const char SYM_LINK_2[] = "/sym_2";

void verify_content(const char file[]) {
    int f = tfs_open(file, TFS_O_CREAT);
    assert(f != -1);

    char buffer[sizeof(FILE_CONTENT)];

    assert(tfs_read(f, buffer, sizeof(FILE_CONTENT)) == sizeof(FILE_CONTENT));
    assert(memcmp(buffer, FILE_CONTENT, sizeof(FILE_CONTENT)) == 0);

    assert(tfs_close(f) == 0);
}

// This test asserts that multiple threads
int main() {
    assert(tfs_init(NULL) != -1);

    // creates the file
    int fhandle = tfs_open(FILE_NAME, TFS_O_CREAT);
    assert(fhandle != -1);

    // writes the content in the file
    assert(tfs_write(fhandle, FILE_CONTENT, sizeof(FILE_CONTENT)) ==
           sizeof(FILE_CONTENT));

    // closes the file
    assert(tfs_close(fhandle) == 0);
    fhandle = tfs_open(FILE_NAME, TFS_O_CREAT);
    assert(fhandle != -1);

    char buffer[sizeof(FILE_CONTENT)];

    // verifies that what was written is in the file.
    assert(tfs_read(fhandle, buffer, sizeof(FILE_CONTENT)) ==
           sizeof(FILE_CONTENT));
    assert(memcmp(buffer, FILE_CONTENT, sizeof(FILE_CONTENT)) == 0);

    assert(tfs_close(fhandle) == 0);

    // confirms that none of these files exist
    assert(tfs_open(SYM_LINK_1, TFS_O_TRUNC) == -1);
    assert(tfs_open(SYM_LINK_2, TFS_O_TRUNC) == -1);

    // creates a symbolic_link to the file
    assert(tfs_sym_link(FILE_NAME, SYM_LINK_1) == 0);

    // creates a symbolic link to the other symbolic link
    assert(tfs_sym_link(SYM_LINK_1, SYM_LINK_2) == 0);

    // confirms that the file content can be reached from both sym links
    verify_content(SYM_LINK_1);
    verify_content(SYM_LINK_2);

    // now we'll unlink the file and try to make a circular link
    assert(tfs_unlink(FILE_NAME) == 0);

    // it should'nt add
    assert(tfs_sym_link(SYM_LINK_2, FILE_NAME) == -1);

    // destroy TécnicoFS
    assert(tfs_destroy() != -1);
    printf("Successful test.\n");
    return 0;
}