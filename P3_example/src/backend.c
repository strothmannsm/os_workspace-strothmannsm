#include "backend.h"

#include <string.h>
#include <time.h>
#include <backend.h>

// Ok, less worse than a RMW, but it isn't very space efficient
// One neat idea would be to make a shared buffer a global so both can use it
// Since we're soooooo not thread safe anyway, it could save some space... kinda?
// Maybe not since it would be a constant 1k being taken up as opposed to the occasional 1k
// Hmm.
bool partial_read(const S16FS_t *fs, void *data, const block_ptr_t block, const unsigned offset, const unsigned bytes) {
    if (fs && data && BLOCK_PTR_VALID(block) && offset < BLOCK_SIZE && bytes) {
        data_block_t buffer;
        if (back_store_read(fs->bs, block, &buffer)) {
            memcpy(data, &buffer + offset, bytes);
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
            memcpy(&buffer + offset, data, bytes);
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
    S16FS_t *fs = (S16FS_t *) calloc(1,sizeof(S16FS_t));
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
            // That was a bad idea, switched to calloc
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
                    result_t scan_results = {true, true, false, 0, 0, 0, 0, 0, 0, NULL};  // hey, cool, I found root!
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
    uint64_t pos; - IF FOUND: Entry position in directory
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
                            res->pos   = i;
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

bool wipe_parent_entry(S16FS_t *const fs, const char *fname, const inode_ptr_t parent_id) {
    if (fs && fname) {
        inode_t dir_inode;
        dir_block_t dir_data;
        if (read_inode(fs, &dir_inode, parent_id) && INODE_IS_TYPE(&dir_inode, FS_DIRECTORY)
            && full_read(fs, &dir_data, dir_inode.data_ptrs[0])) {
            // stripping out fname checks. Hopefully you checked it before getting here
            for (unsigned i = 0; i < DIR_REC_MAX; ++i) {
                if (strncmp(fname, dir_data.entries[i].fname, FS_FNAME_MAX) == 0) {
                    // found it!
                    // time for it to have a bad day
                    dir_data.entries[i].fname[0] = '\0';
                    --dir_data.mdata.size;
                    // Ok, maybe less of a bad day. Less violent than I was expecting.
                    return full_write(fs, &dir_data, dir_inode.data_ptrs[0]);
                }
            }
        }
    }
    return false;
}

// This kills the file. Make sure the info's correct.
bool release_dir(S16FS_t *fs, const char *fname, const inode_ptr_t target_number, const inode_ptr_t parent_number,
                 const block_ptr_t block_number) {
    if (fs && target_number != 0 && block_number != ROOT_DIR_BLOCK) {
        // I'm really just going to assume that the block number is the right one
        dir_block_t target_dir;
        if (full_read(fs, &target_dir, block_number)) {
            if (target_dir.mdata.size == 0) {
                // Cool, can kill dir. We should kill the parent ref first, it's safest.

                // Find the record
                // Which I'm going to implement manually, BUT should really be rolled into scan_directory
                // But I don't want to add something to result_t for the third time
                // ...but it really is better, so I will (result_t.pos) and then toss all this reading
                // ...but then I have to call scan, which loads everything, to get the number
                //    Which then forces me to reload stuff. That's gross.
                // ...Making new function

                if (wipe_parent_entry(fs, fname, parent_number)) {
                    // Target dir is now unhooked from the dir tree
                    // we should kill the inode first, I suppose, because it's safer
                    if (clear_inode(fs, target_number)) {
                        // Now we're just leaking a dir block if anything else fails

                        back_store_release(fs->bs, target_number);
                        // Well that doesn't return a status, so... I guess it worked.
                        // I guess that was a design oversight.
                        // Something to fix next time.
                        return true;
                    }
                }
            }
        }
    }
    return false;
}

bool release_regular(S16FS_t *fs, const char *fname, const inode_ptr_t target_number, const inode_ptr_t parent_number) {
    // ... buckle up.
    inode_t file_inode;
    indir_block_t indirect, dbl_indirect;
    if (fs && target_number != 0) {
        // Uhh, why not a root check... because... idk
        if (wipe_parent_entry(fs, fname, parent_number)) {
            if (read_inode(fs, &file_inode, target_number)) {
                bool success = true;
                if (file_inode.data_ptrs[0] != 0) {
                    // Alrighty. These are the easy ones.
                    for (unsigned i = 0; file_inode.data_ptrs[i] && i < DIRECT_TOTAL; ++i) {
                        back_store_release(fs->bs, file_inode.data_ptrs[i]);
                    }
                    if (file_inode.data_ptrs[6] != 0) {
                        // Oh man.
                        if ((success = full_read(fs, &indirect, file_inode.data_ptrs[6]))) {
                            for (unsigned i = 0; indirect.block_ptrs[i] && i < INDIRECT_TOTAL; ++i) {
                                back_store_release(fs->bs, indirect.block_ptrs[i]);
                            }
                            back_store_release(fs->bs, file_inode.data_ptrs[6]);

                            // Alright, now for the gross one
                            if (file_inode.data_ptrs[7] != 0) {
                                if ((success = full_read(fs, &dbl_indirect, file_inode.data_ptrs[7]))) {
                                    // read the double
                                    for (unsigned dbl_idx = 0;
                                         success && dbl_indirect.block_ptrs[dbl_idx] && dbl_idx < INDIRECT_TOTAL;
                                         ++dbl_idx) {
                                        if ((success = full_read(fs, &indirect, dbl_indirect.block_ptrs[dbl_idx]))) {
                                            // got the indir
                                            for (unsigned i = 0; indirect.block_ptrs[i] && i < INDIRECT_TOTAL; ++i) {
                                                back_store_release(fs->bs, indirect.block_ptrs[i]);
                                            }
                                            back_store_release(fs->bs, dbl_indirect.block_ptrs[dbl_idx]);
                                        }
                                    }
                                    if (success) {
                                        back_store_release(fs->bs, file_inode.data_ptrs[7]);
                                    }
                                }
                            }
                        }
                    }
                }
                return success ? clear_inode(fs, target_number) : false;
            }
        }
    }
    return false;
}

bool fd_valid(const S16FS_t *const fs, int fd) {
    return fs && fd >= 0 && fd < DESCRIPTOR_MAX && bitmap_test(fs->fd_table.fd_status, fd);
}

ssize_t overwrite_file(S16FS_t *fs, const inode_t *inode, const size_t position, const void *data, size_t nbyte) {
    // Only overwriting data, no need to mess with allocation.
    if (fs && inode && data) {
        size_t first_block = position / BLOCK_SIZE;
        size_t last_block  = (position + nbyte - 1) / BLOCK_SIZE;
        // -1 becasue it'll go too for when it's on the boundary
        dyn_array_t *block_list = get_blocks(fs, inode, first_block, last_block);
        data_block_t data_block;
        ssize_t data_written = 0;
        block_ptr_t working_block;
        bool success = true;
        if (block_list) {
            // potentially write partial block
            if ((success = dyn_array_extract_front(block_list, &working_block))) {
                unsigned offset = position & (BLOCK_SIZE - 1);
                unsigned length = (dyn_array_empty(block_list) ? nbyte : BLOCK_SIZE - offset);
                if ((success = back_store_read(fs->bs, working_block, &data_block))) {
                    memcpy(data_block + offset, data, length);
                    if ((success = back_store_write(fs->bs, working_block, &data_block))) {
                        data = INCREMENT_VOID(data, length);
                        data_written += length;
                    }
                }
            }
            // loop the middle
            while (success && dyn_array_size(block_list) > 1
                   && (success = dyn_array_extract_front(block_list, &working_block))) {
                if ((success = back_store_write(fs->bs, working_block, data))) {
                    data = INCREMENT_VOID(data, BLOCK_SIZE);
                    data_written += BLOCK_SIZE;
                }
            }
            // potentially write partial block, but always at front of block
            if (success && dyn_array_size(block_list) == 1
                && (success = dyn_array_extract_front(block_list, &working_block))) {
                unsigned length = (position + nbyte) & (BLOCK_SIZE - 1);
                if (length == 0) {
                    length = 1024;
                }
                // I find it funny that it's really hard to tell the difference when you need to
                // write 1024 or 0
                // Thankfully, you can't be here if you're done writing.
                if ((success = back_store_read(fs->bs, working_block, &data_block))) {
                    memcpy(data_block, data, length);
                    if ((success = back_store_write(fs->bs, working_block, &data_block))) {
                        data = INCREMENT_VOID(data, length);
                        data_written += length;
                    }
                }
            }
            dyn_array_destroy(block_list);
            return data_written;
        }
    }
    return -1;
}

dyn_array_t *get_blocks(const S16FS_t *fs, const inode_t *inode, const size_t first, const size_t last) {
    if (inode && last < (DIRECT_TOTAL + INDIRECT_TOTAL + INDIRECT_TOTAL * INDIRECT_TOTAL)) {
        dyn_array_t *results = dyn_array_create(last - first, sizeof(block_ptr_t), NULL);
        size_t i             = first;
        bool success         = true;
        indir_block_t single_indir, dbl_indir;
        for (; i < DIRECT_TOTAL && i <= last && success; ++i) {
            success = dyn_array_push_back(results, &inode->data_ptrs[i]);
        }
        if (i <= last && i < (DIRECT_TOTAL + INDIRECT_TOTAL) && success
            && (success = full_read(fs, &single_indir, inode->data_ptrs[6]))) {
            for (size_t idx = (i - DIRECT_TOTAL); i < (DIRECT_TOTAL + INDIRECT_TOTAL) && i <= last && success;
                 ++i, ++idx) {
                success = dyn_array_push_back(results, single_indir.block_ptrs + idx);
            }
        }
        if (i <= last && success && (success = full_read(fs, &dbl_indir, inode->data_ptrs[7]))) {
            for (size_t dbl_idx = ((i - (DIRECT_TOTAL + INDIRECT_TOTAL)) / INDIRECT_TOTAL); i <= last && success;
                 ++dbl_idx) {
                success = full_read(fs, &single_indir, dbl_indir.block_ptrs[dbl_idx]);
                for (size_t idx = ((i - (DIRECT_TOTAL + INDIRECT_TOTAL)) - (dbl_idx * INDIRECT_TOTAL));
                     idx < INDIRECT_TOTAL && i <= last && success; ++i, ++idx) {
                    success = dyn_array_push_back(results, single_indir.block_ptrs + idx);
                }
            }
        }
        return results;
    }
    return NULL;
}

ssize_t extend_file(S16FS_t *fs, inode_t *inode, inode_ptr_t inode_number, size_t new_len) {
    if (inode) {
        uint32_t current_size      = inode->mdata.size;
        size_t cur_block_total     = (current_size / BLOCK_SIZE) + (current_size & (BLOCK_SIZE - 1) ? 1 : 0);
        size_t new_block_count     = (new_len / BLOCK_SIZE) + (new_len & (BLOCK_SIZE - 1) ? 1 : 0);
        indir_block_t single_indir = {{0}}, double_indir = {{0}};
        if (cur_block_total == new_block_count) {
            // Just stop. Now. Please.
            return 0;
        }
        bool success = true;
        block_ptr_t new_block;
        // direct
        if (cur_block_total < DIRECT_TOTAL) {
            for (; cur_block_total < DIRECT_TOTAL && success && cur_block_total < new_block_count;) {
                new_block = back_store_allocate(fs->bs);
                if ((success = new_block != 0)) {
                    inode->data_ptrs[cur_block_total] = new_block;
                    ++cur_block_total;
                }
            }
        }
        // indirect
        if (success && cur_block_total < new_block_count && cur_block_total < (DIRECT_TOTAL + INDIRECT_TOTAL)) {
            if (inode->data_ptrs[6] == 0) {
                // need to allocate indirect
                new_block = back_store_allocate(fs->bs);
                if ((success = new_block != 0)) {
                    inode->data_ptrs[6] = new_block;
                }
            } else {
                success = back_store_read(fs->bs, inode->data_ptrs[6], &single_indir);
            }
            if (success) {
                for (size_t i = cur_block_total - DIRECT_TOTAL;
                     i < INDIRECT_TOTAL && success && cur_block_total < new_block_count; ++i) {
                    new_block = back_store_allocate(fs->bs);
                    if ((success = new_block != 0)) {
                        single_indir.block_ptrs[i] = new_block;
                        ++cur_block_total;
                    }
                }
                // write indir back
                success = back_store_write(fs->bs, inode->data_ptrs[6], &single_indir);
            }
        }
        // dbl
        if (success && cur_block_total < new_block_count) {
            if (inode->data_ptrs[7] == 0) {
                new_block = back_store_allocate(fs->bs);
                if ((success = new_block != 0)) {
                    inode->data_ptrs[7] = new_block;
                }
            } else {
                success = back_store_read(fs->bs, inode->data_ptrs[7], &double_indir);
            }
            for (size_t dbl_idx = (cur_block_total - DIRECT_TOTAL - INDIRECT_TOTAL) / INDIRECT_TOTAL;
                 cur_block_total < new_block_count && success && dbl_idx < INDIRECT_TOTAL; ++dbl_idx) {
                if (double_indir.block_ptrs[dbl_idx] == 0) {
                    new_block = back_store_allocate(fs->bs);
                    if ((success = new_block != 0)) {
                        double_indir.block_ptrs[dbl_idx] = new_block;
                        memset(&single_indir, 0x00, sizeof(indir_block_t));
                    }
                } else {
                    success = back_store_read(fs->bs, double_indir.block_ptrs[dbl_idx], &single_indir);
                }
                for (size_t idx = (cur_block_total - DIRECT_TOTAL - INDIRECT_TOTAL) & (INDIRECT_TOTAL - 1);
                     cur_block_total < new_block_count && success && idx < INDIRECT_TOTAL; ++idx) {
                    new_block = back_store_allocate(fs->bs);
                    if ((success = new_block != 0)) {
                        single_indir.block_ptrs[idx] = new_block;
                        ++cur_block_total;
                    }
                }
                success = back_store_write(fs->bs, double_indir.block_ptrs[dbl_idx], &single_indir);
            }
            success = back_store_write(fs->bs, inode->data_ptrs[7], &double_indir);
        }
        // flush inode and return
        inode->mdata.size = (new_block_count == cur_block_total ? new_len : (cur_block_total * BLOCK_SIZE));
        return write_inode(fs, inode, inode_number) ? inode->mdata.size : -1;
    }
    return -1;
}

void release_fds(S16FS_t *fs, int inode_number) {
    if (fs) {
        for(size_t i = 0; i < DESCRIPTOR_MAX; ++i){
            // Oh god 256 if statements. I am so sorry.
            if (fs->fd_table.fd_inode[i] == inode_number) {
                fs->fd_table.fd_inode[i] = 0;
                bitmap_reset(fs->fd_table.fd_status,i);
            }
        }
    }
}
