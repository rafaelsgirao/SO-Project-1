#include "operations.h"
#include "config.h"
#include "state.h"
#include <stdbool.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "betterassert.h"

tfs_params tfs_default_params() {
    tfs_params params = {
        .max_inode_count = 64,
        .max_block_count = 1024,
        .max_open_files_count = 16,
        .block_size = 1024,
    };
    return params;
}

int tfs_init(tfs_params const *params_ptr) {
    tfs_params params;
    if (params_ptr != NULL) {
        params = *params_ptr;
    } else {
        params = tfs_default_params();
    }

    if (state_init(params) != 0) {
        return -1;
    }

    // create root inode
    int root = inode_create(T_DIRECTORY);
    if (root != ROOT_DIR_INUM) {
        return -1;
    }

    return 0;
}

int tfs_destroy() {
    if (state_destroy() != 0) {
        return -1;
    }
    return 0;
}

static bool valid_pathname(char const *name) {
    return name != NULL && strlen(name) > 1 && name[0] == '/';
}

/**
 * Looks for a file.
 *
 * Note: as a simplification, only a plain directory space (root directory only)
 * is supported.
 *
 * Input:
 *   - name: absolute path name
 *   - root_inode: the root directory inode
 * Returns the inumber of the file, -1 if unsuccessful.
 */
static int tfs_lookup(char const *name, inode_t const *root_inode) {
    // TODO: assert that root_inode is the root directory
    if (!valid_pathname(name)) {
        return -1;
    }

    // skip the initial '/' character
    name++;

    return find_in_dir(root_inode, name);
}

int tfs_open(char const *name, tfs_file_mode_t mode) {
    // Checks if the path name is valid
    if (!valid_pathname(name)) {
        return -1;
    }

    inode_t *root_dir_inode = inode_get(ROOT_DIR_INUM);
    ALWAYS_ASSERT(root_dir_inode != NULL,
                  "tfs_open: root dir inode must exist");
    int inum = tfs_lookup(name, root_dir_inode);
    size_t offset;

    if (inum >= 0) {
        // The file already exists
        inode_t *inode = inode_get(inum);
        ALWAYS_ASSERT(inode != NULL,
                      "tfs_open: directory files must have an inode");

        // Truncate (if requested)
        if (mode & TFS_O_TRUNC) {
            if (inode->i_size > 0) {
                data_block_free(inode->i_data_block);
                inode->i_size = 0;
            }
        }
        // Determine initial offset
        if (mode & TFS_O_APPEND) {
            offset = inode->i_size;
        } else {
            offset = 0;
        }
    } else if (mode & TFS_O_CREAT) {
        // The file does not exist; the mode specified that it should be created
        // Create inode
        inum = inode_create(T_FILE);
        if (inum == -1) {
            return -1; // no space in inode table
        }

        // Add entry in the root directory
        if (add_dir_entry(root_dir_inode, name + 1, inum) == -1) {
            inode_delete(inum);
            return -1; // no space in directory
        }

        offset = 0;
    } else {
        return -1;
    }

    // Finally, add entry to the open file table and return the corresponding
    // handle
    return add_to_open_file_table(inum, offset);

    // Note: for simplification, if file was created with TFS_O_CREAT and there
    // is an error adding an entry to the open file table, the file is not
    // opened but it remains created
}

int tfs_sym_link(char const *target, char const *link_name) {
    if (!valid_pathname(link_name) || !valid_pathname(target))
        return -1;

    int i_target_num = tfs_lookup(target, inode_get(ROOT_DIR_INUM));
    if (i_target_num == -1)
        return -1;

    inode_t *i_target = inode_get(i_target_num);
    if (i_target == NULL)
        return -1;

    int i_link_number = inode_create(T_SYM_LINK);
    if (i_link_number == -1)
        return -1;

    inode_t *i_link = inode_get(i_link_number);
    if (i_link == NULL)
        return -1;

    // initializes link's inode
    strcpy(i_link->i_target_d_name, target);

    const char *link_sub = link_name + 1;
    if (add_dir_entry(inode_get(ROOT_DIR_INUM), link_sub, i_link_number) == -1)
        return -1;

    return 0;
}

int tfs_link(char const *target, char const *link_name) {
    if (!valid_pathname(target) || !valid_pathname(link_name))
        return -1;

    // we must remove the '/' when adding to dir entry
    const char *link_sub = link_name + 1;

    inode_t *iroot = inode_get(ROOT_DIR_INUM);
    ALWAYS_ASSERT(iroot != NULL, "tfs_link: failed to find root dir inode");

    int target_inumber = tfs_lookup(target, iroot);
    if (target_inumber == -1)
        return -1;

    inode_t *itarget = inode_get(target_inumber);
    if (itarget == NULL || itarget->i_node_type == T_SYM_LINK ||
        add_dir_entry(iroot, link_sub, target_inumber) == -1)
        return -1;

    itarget->i_links++;

    return 0;
}

int tfs_close(int fhandle) {
    open_file_entry_t *file = get_open_file_entry(fhandle);
    if (file == NULL) {
        return -1; // invalid fd
    }

    remove_from_open_file_table(fhandle);

    return 0;
}

ssize_t tfs_write(int fhandle, void const *buffer, size_t to_write) {
    open_file_entry_t *file = get_open_file_entry(fhandle);
    if (file == NULL) {
        return -1;
    }

    //  From the open file table entry, we get the inode
    inode_t *inode = inode_get(file->of_inumber);
    ALWAYS_ASSERT(inode != NULL, "tfs_write: inode of open file deleted");

    // Determine how many bytes to write
    size_t block_size = state_block_size();
    if (to_write + file->of_offset > block_size) {
        to_write = block_size - file->of_offset;
    }

    if (to_write > 0) {
        if (inode->i_size == 0) {
            // If empty file, allocate new block
            int bnum = data_block_alloc();
            if (bnum == -1) {
                return -1; // no space
            }

            inode->i_data_block = bnum;
        }

        void *block = data_block_get(inode->i_data_block);
        ALWAYS_ASSERT(block != NULL, "tfs_write: data block deleted mid-write");

        // Perform the actual write
        memcpy(block + file->of_offset, buffer, to_write);

        // The offset associated with the file handle is incremented accordingly
        file->of_offset += to_write;
        if (file->of_offset > inode->i_size) {
            inode->i_size = file->of_offset;
        }
    }

    return (ssize_t)to_write;
}

ssize_t tfs_read(int fhandle, void *buffer, size_t len) {
    open_file_entry_t *file = get_open_file_entry(fhandle);
    if (file == NULL) {
        return -1;
    }

    // From the open file table entry, we get the inode
    inode_t const *inode = inode_get(file->of_inumber);
    ALWAYS_ASSERT(inode != NULL, "tfs_read: inode of open file deleted");

    // Determine how many bytes to read
    size_t to_read = inode->i_size - file->of_offset;
    if (to_read > len) {
        to_read = len;
    }

    if (to_read > 0) {
        void *block = data_block_get(inode->i_data_block);
        ALWAYS_ASSERT(block != NULL, "tfs_read: data block deleted mid-read");

        // Perform the actual read
        memcpy(buffer, block + file->of_offset, to_read);
        // The offset associated with the file handle is incremented accordingly
        file->of_offset += to_read;
    }

    return (ssize_t)to_read;
}

int tfs_unlink(char const *target) {
    if (!valid_pathname(target))
        return -1;

    const char *target_sub = target + 1;

    int i_target_num = tfs_lookup(target, inode_get(ROOT_DIR_INUM));
    inode_t *i_target = inode_get(i_target_num);
    if (i_target == NULL)
        return -1;

    if (i_target->i_links - 1 <= 0) {
        if (i_target->i_node_type != T_SYM_LINK)
            data_block_free(i_target->i_data_block);
        inode_delete(i_target_num);
    } else {
        i_target->i_links--;
        clear_dir_entry(inode_get(ROOT_DIR_INUM), target_sub);
    }

    return 0;
}

int tfs_copy_from_external_fs(char const *source_path, char const *dest_path) {
    // Open source
    FILE *source = fopen(source_path, "r");
    if (source == NULL) {
        return -1;
    }

    // Open dest
    tfs_file_mode_t open_mode = TFS_O_CREAT | TFS_O_CREAT;
    int dest = tfs_open(dest_path, open_mode);
    if (dest == -1) {
        // Close external file
        fclose(source);
        return -1;
    }

    // Read from source, write to dest
    char buffer[EXT_BUFFER];
    size_t bytes_read = fread(buffer, 1, EXT_BUFFER, source);

    while (bytes_read > 0) {
        tfs_write(dest, buffer, bytes_read);
        bytes_read = fread(buffer, 1, EXT_BUFFER, source);
    }
    // Close files
    tfs_close(dest);
    fclose(source);

    return 0;
}
