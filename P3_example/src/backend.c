#include "backend.h"

#include <string.h>
#include <time.h>

// Ok, less worse than a RMW, but it isn't very space efficient
// One neat idea would be to make a shared buffer a global so both can use it
// Since we're soooooo not thread safe anyway, it could save some space... kinda?
// Maybe not since it would be a constant 1k being taken up as opposed to the occasional 1k
// Hmm.
bool partial_read(const S16FS_t *fs, void *data, const block_ptr_t block, const unsigned offset, const unsigned bytes) {
    if (fs && data && BLOCK_PTR_VALID(block) && offset < BLOCK_SIZE && bytes) {
        data_block_t buffer;
        if (back_store_read(fs->bs, block, &buffer)) {
            memcpy(data, INCREMENT_VOID(&buffer, offset), bytes);
            return true;
        }
    }
    return false;
}

// THE DREADED READ MODIFY WRITE. Avoid it at all costs. Generally. Unless it's just the one thing. That's cool.
bool partial_write(S16FS_t *fs, const void *data, const block_ptr_t block, const unsigned offset,
                   const unsigned bytes) {
    if (fs && data && BLOCK_PTR_VALID(block) && offset < BLOCK_SIZE && bytes) {
        // if (bytes == 0) return true; // just in case my logic gets weird somewhere
        // but that won't "allow" ofset = 1024 and bytes = 0
        // Scratch that, return false. If it actually happens, it should be reported
        data_block_t buffer;
        if (back_store_read(fs->bs, block, &buffer)) {
            memcpy(INCREMENT_VOID(&buffer, offset, data, bytes);
            return back_store_write(fs->bs, block, &buffer);
        }
    }
    return false;
}

bool read_inode(const S16FS_t *fs, void *data, const inode_ptr_t inode_number) {
    if (fs && data) {
        inode_t buffer[INODES_PER_BOCK];
        if (back_store_read(fs->bs, INODE_TO_BLOCK(inode_number), buffer)) {
            memcpy(data, &buffer[INODE_INNER_IDX(inode_number)], sizeof(inode_t));
            return true;
        }
    }
    return false;
}

bool write_inode(S16FS_t *fs, const void *data, const inode_ptr_t inode_number) {
    if (fs && data) {  // checking if the inode number is valid is a tautology :/
        inode_t buffer[INODES_PER_BOCK];
        if (back_store_read(fs->bs, INODE_TO_BLOCK(inode_number), buffer)) {
            memcpy(&buffer[INODE_INNER_IDX(inode_number)], data, sizeof(inode_t));
            return back_store_write(fs->bs, INODE_TO_BLOCK(inode_number), buffer);
        }
    }
    return false;
}

bool clear_inode(S16FS_t *fs, const inode_ptr_t inode_number) {
    // Just going to blank the first fname character.
    // Allows for easier post-mortem debugging than completely blanking it
    if (fs) {
        inode_t buffer[INODES_PER_BOCK];
        if (back_store_read(fs->bs, INODE_TO_BLOCK(inode_number), buffer)) {
            buffer[INODE_INNER_IDX(inode_number)].fname[0] = '\0';
            return back_store_write(fs->bs, INODE_TO_BLOCK(inode_number), buffer);
        }
    }
    return false;
}

// might as well make versions that do whole blocks. Better encapsulation?
// All calls are verified a bit more before happening, which is good.
bool full_read(const S16FS_t *fs, void *data, const block_ptr_t block) {
    if (fs && data) {  // you can read from the inode table...
        return back_store_read(fs->bs, block, data);
    }
    return false;
}

bool full_write(S16FS_t *fs, const void *data, const block_ptr_t block) {
    if (fs && data && block >= DATA_BLOCK_OFFSET) {  // but you can't write to it. Not in bulk.
                                                     // there is NO reason to do a bulk write to the inode table
        return back_store_write(fs->bs, block, data);
    }
    return false;
}


S16FS_t *ready_file(const char *path, const bool format) {
    S16FS_t *fs = (S16FS_t *) malloc(sizeof(S16FS_t));
    if (fs) {
        if (format) {
            // get inode table
            // format root
            // That's it?

            // oh, also, ya know, make the back store object. oops.
            fs->bs = back_store_create(path);
            if (fs->bs) {
                bool valid = true;
                // + 1 to snag the root dir block because lazy
                for (int i = INODE_BLOCK_OFFSET; i < (DATA_BLOCK_OFFSET + 1) && valid; ++i) {
                    valid &= back_store_request(fs->bs, i);
                }
                // inode table is already blanked because back_store blanks all data (woo)
                if (valid) {
                    // I'm actually not sure how to do this
                    // It's going to look like a mess
                    uint32_t right_now = time(NULL);
                    inode_t root_inode = {"/",
                                          {0, 0777, right_now, right_now, right_now, 0, FS_DIRECTORY, {0}},
                                          {DATA_BLOCK_OFFSET, 0, 0, 0, 0, 0, 0, 0}};
                    // fname technically invalid, but it's root so deal
                    // mdata actually might not be used in a dir record. Idk.
                    // block pointer set, rest are invalid
                    // break point HERE to make sure that constructed right
                    valid &= write_inode(fs, &root_inode, 0);
                }
                if (!valid) {
                    // weeeeeeeh
                    back_store_close(fs->bs);
                    fs->bs = NULL;  // just set it and trigger the free elsewhere
                }
            }
        } else {
            fs->bs = back_store_open(path);
            // ... that's it?
        }
        if (fs->bs) {
            fs->fd_table.fd_status = bitmap_create(DESCRIPTOR_MAX);
            // Eh, won't bother blanking out tables, since that's the point of the bitmap
            if (fs->fd_table.fd_status) {
                return fs;
            }
        }
        free(fs);
    }
    return NULL;
}

/*
hunts down the requested file, if it exists, filling out all sorts of little bits of data

typedef struct {
    bool success; - Did the operation complete? (generally just parameter issues)
    bool found; - Did we find the file? Does it exist?
    bool valid; - N/A
    inode_ptr_t inode; - IF FOUND: inode of file
    inode_ptr_t parent; - IF FOUND: inode of parent directory of file
    block_ptr_t block; - IF FOUND AND TYPE DIRECTORY: Dir block for directory
    file_t type; - IF FOUND: Type of file
    uint64_t total; - N/A
    void *data; - Pointer to last token parsed (in given path string)
                    Either the filename, or the token we died on
} result_t;

*/

void locate_file(const S16FS_t *const fs, const char *abs_path, result_t *res) {
    if (res) {
        memset(res, 0x00, sizeof(result_t));  // IMMEDIATELY blank it
        if (fs && abs_path) {
            // need to get the token processing loop started (oh boy!)
            const size_t path_len = strnlen(abs_path, FS_PATH_MAX);
            if (path_len != 0 && abs_path[0] == '/' && path_len < FS_PATH_MAX) {
                // ok, path is something we should at least bother trying to look at
                char *path_copy = strndup(abs_path, path_len);
                if (path_copy) {
                    result_t scan_results = {true, true, false, 0, 0, 0, 0, 0, NULL};  // hey, cool, I found root!
                    const char *delims    = "/";
                    res->success          = true;
                    res->found            = true;  // I'm going to assume it all works out, don't go making me a liar
                    res->inode            = 0;
                    res->block            = ROOT_DIR_BLOCK;
                    res->type             = FS_DIRECTORY;
                    res->data             = (void *) abs_path;
                    char *token           = strtok(path_copy, delims);
                    // Hardcoding results for root in case there aren't any tokens (path was "/")

                    while (token) {
                        // update the dir pointer
                        res->data = (void *) (abs_path + (token - path_copy));

                        // Cool. Does the next token exist in the current directory?
                        scan_directory(fs, token, scan_results.inode, &scan_results);

                        if (scan_results.success && scan_results.found) {
                            // Good. It existed. Cycle.
                            res->parent = scan_results.parent;
                            res->inode  = scan_results.inode;
                            token       = strtok(NULL, delims);
                            continue;
                        }
                        // welp. Something's broken. File not found.
                        res->found = false;
                        break;
                    }
                    if (res->found) {
                        inode_t found_file;
                        if (read_inode(fs, &found_file, res->inode)) {
                            res->type = found_file.mdata.type;
                            if (res->type == FS_DIRECTORY) {
                                res->block = found_file.data_ptrs[0];
                            }
                            free(path_copy);
                            return;
                        }
                        // back_store ate it I guess? That's bad.
                        // What do we do now? (die.)
                        memset(res, 0x00, sizeof(result_t));  // all that work for nothing
                    }
                    free(path_copy);
                }
            }
        }
    }
}

/*
Flips through the specified directory, finding the specified file (hopefully)

typedef struct {
    bool success; - Did the operation complete? (generally just parameter issues) Was it a dir?
    bool found; - IF VALID: Did we find the file? Does it exist?
    bool valid; - Was the filename valid?
    inode_ptr_t inode; - IF FOUND: inode of file
    inode_ptr_t parent; - IF SUCCESS: Literally the inode number you fed us
    block_ptr_t block; - IF SUCCESS: Data block of directory.
                            Shouldn't never need it, but we know it, so we'll share
    file_t type; - IF FOUND: type of the file found
    uint64_t total; - IF SUCCESS: Number of files in the given directory
    void *data; - N/A
} result_t;
*/
void scan_directory(const S16FS_t *const fs, const char *fname, const inode_ptr_t inode, result_t *res) {
    if (res) {
        memset(res, 0x00, sizeof(result_t));
        if (fs && fname) {
            // inode number is always valid - tbh, that may mask errors and could be considered bad
            inode_t dir_inode;
            dir_block_t dir_data;
            if (read_inode(fs, &dir_inode, inode) && INODE_IS_TYPE(&dir_inode, FS_DIRECTORY)
                && full_read(fs, &dir_data, dir_inode.data_ptrs[0])) {
                res->success = true;
                res->block   = dir_inode.data_ptrs[0];
                res->total   = dir_data.mdata.size;
                res->parent  = inode;
                // let's validate the fname
                const size_t fname_len = strnlen(fname, FS_FNAME_MAX);
                if (fname_len != 0 && fname_len < FS_FNAME_MAX) {
                    // Alrighty, we got the inode and block read in.
                    // fname is vaguely validated
                    res->valid = true;
                    for (unsigned i = 0; i < DIR_REC_MAX; ++i) {
                        if (strncmp(fname, dir_data.entries[i].fname, FS_FNAME_MAX) == 0) {
                            // found it!
                            res->found = true;
                            res->inode = dir_data.entries[i].inode;
                            return;
                        }
                    }
                }
            }
        }
    }
}

// Just what it sounds like. 0 on error
inode_ptr_t find_free_inode(const S16FS_t *const fs) {
    if (fs) {
        inode_t inode_block[INODES_PER_BOCK];
        inode_ptr_t free_inode = 0;
        for (unsigned blk = INODE_BLOCK_OFFSET; blk < DATA_BLOCK_OFFSET; ++blk) {
            if (full_read(fs, &inode_block, blk)) {
                for (unsigned i = 0; i < INODES_PER_BOCK; ++i, ++free_inode) {
                    if (inode_block[i].fname[0] == '\0') {
                        return free_inode;
                        // potentially a conversion warning because integer truncation/depromotion
                    }
                }
            } else {
                return 0;  // :/
            }
        }
    }
    return 0;
}
