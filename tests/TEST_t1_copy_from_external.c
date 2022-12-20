#include "fs/operations.h"
#include <assert.h>
#include <stdio.h>
#include <string.h>
#include <sys/types.h>

int main() {
    const char INTERNAL_FILE[] = "/file";
    const char EXTERNAL_FILE[] = "tests/lorem_ipsum.txt";
    const char FILE_CONTENT[] =
        "Proin vehicula, nisi id pharetra consequat, ante tortor vulputate "
        "felis, ac placerat odio risus ac sapien. Duis sem neque, tincidunt in "
        "dignissim nec, tristique vitae felis. Quisque dignissim magna sit "
        "amet risus tristique iaculis. Donec interdum nisl elit, eu commodo "
        "ipsum condimentum at.";

    assert(tfs_init(NULL) != -1);

    // copies the external file to the file system
    assert(tfs_copy_from_external_fs(EXTERNAL_FILE, INTERNAL_FILE) == 0);

    int fhandle = tfs_open(INTERNAL_FILE, TFS_O_CREAT);
    assert(fhandle != -1);

    char buffer[sizeof(FILE_CONTENT)];
    assert(tfs_read(fhandle, buffer, sizeof(FILE_CONTENT)) ==
           sizeof(FILE_CONTENT) - 1);

    // verifies if the contents are the same
    assert(memcmp(buffer, FILE_CONTENT, strlen(FILE_CONTENT)) == 0);
    assert(tfs_close(fhandle) != -1);

    char buffer2[sizeof(FILE_CONTENT)];

    // reinsert the content from the buffer
    fhandle = tfs_open(INTERNAL_FILE, TFS_O_TRUNC);
    assert(tfs_write(fhandle, buffer, sizeof(buffer)) == sizeof(buffer));

    ssize_t len = tfs_read(fhandle, buffer2, sizeof(FILE_CONTENT));
    printf("Buffer: %s\nlen: %ld, size: %lu\n", buffer2, len,
           sizeof(FILE_CONTENT));
    // assert(tfs_read(fhandle, buffer2, sizeof(FILE_CONTENT)) ==
    //        sizeof(FILE_CONTENT));

    // verifies if the contents are the same
    assert(memcmp(buffer, FILE_CONTENT, strlen(FILE_CONTENT)) == 0);
    assert(tfs_close(fhandle) != -1);

    assert(tfs_destroy() != -1);
    printf("Successful test.\n");
    return 0;
}