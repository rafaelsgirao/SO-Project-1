#include "fs/operations.h"
#include <assert.h>
#include <pthread.h>
#include <stdbool.h>
#include <stdint.h>
#include <stdio.h>
#include <string.h>
#include <unistd.h>

const char INVALID_LINK_NAME[] = "!!!INVALD";

const char FILE_CONTENT[] = "Strike me down and I will become more powerful "
                            "than you could possibly imagine.";

const char ORIGINAL_FILE[] = "/i_am_your_father";
const char HARD_LINK_1[] = "/h1";
const char HARD_LINK_2[] = "/h2";
const char SYM_LINK[] = "/sym";

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

    verify_file_inexistent(ORIGINAL_FILE);

    // cannot create links to inexistent files
    assert(tfs_link(ORIGINAL_FILE, HARD_LINK_1) == -1);

    int f = tfs_open(ORIGINAL_FILE, TFS_O_CREAT);
    assert(f != -1);

    assert(tfs_write(f, FILE_CONTENT, sizeof(FILE_CONTENT)) ==
           sizeof(FILE_CONTENT));
    assert(tfs_close(f) == 0);

    verify_content(ORIGINAL_FILE);

    // cannot create links with invalid path names
    assert(tfs_link(ORIGINAL_FILE, INVALID_LINK_NAME) == -1);

    // links the hard link within the original file
    assert(tfs_link(ORIGINAL_FILE, HARD_LINK_1) == 0);

    verify_content(HARD_LINK_1);

    // create a symbolic link
    assert(tfs_sym_link(HARD_LINK_1, SYM_LINK) == 0);

    // cannot create hard links to symbolic links
    assert(tfs_link(SYM_LINK, HARD_LINK_2) == -1);

    // can create links to links
    assert(tfs_link(HARD_LINK_1, HARD_LINK_2) == 0);

    // all links should have the same content
    verify_content(HARD_LINK_1);
    verify_content(HARD_LINK_2);
    verify_content(SYM_LINK);

    // destroy TécnicoFS
    assert(tfs_destroy() != -1);
    printf("Successful test.\n");
    return 0;
}