#include <back_store.h>
#include <bitmap.h>

#include <stdint.h>
#include <stdio.h>
#include <time.h>

#include "S16FS.h"

// There's just so much. SO. MUCH.
// Last time it was all in the file.
// There was just so much
// It messes up my autocomplete, but whatever.
#include "backend.h"

#define FD_VALID(fd) ((fd) >= 0 && (fd) < DESCRIPTOR_MAX)

void get_data_block_ptrs(S16FS_t *fs, inode_t *f_inode, size_t position, size_t n_blocks, block_ptr_t *ptrs);
void print_file(S16FS_t *fs, inode_t *f_inode);

///
/// Formats (and mounts) an S16FS file for use
/// \param fname The file to format
/// \return Mounted S16FS object, NULL on error
///
S16FS_t *fs_format(const char *path) {
    return ready_file(path, true);
}

///
/// Mounts an S16FS object and prepares it for use
/// \param fname The file to mount
/// \return Mounted F16FS object, NULL on error
///
S16FS_t *fs_mount(const char *path) {
    return ready_file(path, false);
}

///
/// Unmounts the given object and frees all related resources
/// \param fs The S16FS object to unmount
/// \return 0 on success, < 0 on failure
///
int fs_unmount(S16FS_t *fs) {
    if (fs) {
        back_store_close(fs->bs);
        bitmap_destroy(fs->fd_table.fd_status);
        free(fs);
        return 0;
    }
    return -1;
}

///
/// Creates a new file at the specified location
///   Directories along the path that do not exist are not created
/// \param fs The S16FS containing the file
/// \param path Absolute path to file to create
/// \param type Type of file to create (regular/directory)
/// \return 0 on success, < 0 on failure
///
int fs_create(S16FS_t *fs, const char *path, file_t type) {
    if (fs && path) {
        if (type == FS_REGULAR || type == FS_DIRECTORY) {
            // WHOOPS. Should make sure desired file doesn't already exist.
            // Just going to jam it here.
            result_t file_status;
            locate_file(fs, path, &file_status);
            if (file_status.success && !file_status.found) {
                // alrighty. Need to find the file. And by the file I mean the parent.
                // locate_file doesn't really handle finding the parent if the file doesn't exist
                // So I can't just dump finding this file. Have to search for parent.

                // So, kick off the file finder. If it comes back with the right flags
                // Start checking if we have inodes, the parent exists, a directory, not full
                // if it's a dir check if we have a free block.
                // Fill it all out, update parent, etc. Done!
                const size_t path_len = strnlen(path, FS_PATH_MAX);
                if (path_len != 0 && path[0] == '/' && path_len < FS_PATH_MAX) {
                    // path string is probably ok.
                    char *path_copy, *fname_copy;
                    // this breaks if it's a file at root, since we remove the slash
                    // locate_file treats it as an error
                    // Old version just worked around if if [0] was '\0'
                    // Ideally, I could just ask strndup to allocate an extra byte
                    // Then I can just shift the fname down a byte and insert the NUL there
                    // But strndup doesn't allocate the size given, it seems
                    // So we gotta go manual. Don't think this snippet will be needed elsewhere
                    // Need a malloc, memcpy, then some manual adjustment
                    // path_copy  = strndup(path, path_len);  // I checked, it's not +1. yay MallocScribble
                    path_copy = (char *) calloc(1, path_len + 2);  // NUL AND extra space
                    memcpy(path_copy, path, path_len);
                    fname_copy = strrchr(path_copy, '/');
                    if (fname_copy) {  // CANNOT be null, since we validated [0] as a possibility... but just in case
                        //*fname_copy = '\0';  // heh, split strings, now I have a path to parent AND fname
                        ++fname_copy;
                        const size_t fname_len = path_len - (fname_copy - path_copy);
                        memmove(fname_copy + 1, fname_copy, fname_len + 1);
                        fname_copy[0] = '\0';  // string is split into abs path (now with slash...) and fname
                        ++fname_copy;

                        if (fname_len != 0 && fname_len < (FS_FNAME_MAX - 1)) {
                            // alrighty. Hunt down parent dir.
                            // check it's actually a dir. (ooh, add to result_t!)
                            locate_file(fs, path_copy, &file_status);
                            if (file_status.success && file_status.found && file_status.type == FS_DIRECTORY) {
                                // parent exists, is a directory. Cool.
                                // (added block to locate_file if file is a dir. Handy.)
                                dir_block_t parent_dir;
                                inode_t new_inode;
                                dir_block_t new_dir;
                                uint32_t now = time(NULL);
                                // load dir, check it has space.
                                if (full_read(fs, &parent_dir, file_status.block)
                                    && parent_dir.mdata.size < DIR_REC_MAX) {
                                    // try to grab all new resources (inode, optionally data block)
                                    // if we get all that, commit it.
                                    inode_ptr_t new_inode_idx = find_free_inode(fs);
                                    if (new_inode_idx != 0) {
                                        bool success            = false;
                                        block_ptr_t new_dir_ptr = 0;
                                        switch (type) {
                                            case FS_REGULAR:
                                                // We're all good.
                                                new_inode = (inode_t){
                                                    {0},
                                                    {0, 0777, now, now, now, file_status.inode, FS_REGULAR, {0}},
                                                    {0}};
                                                strncpy(new_inode.fname, fname_copy, fname_len + 1);
                                                // I'm so deep now that my formatter is very upset with every line
                                                // inode = ready
                                                success = write_inode(fs, &new_inode, new_inode_idx);
                                                // Uhh, if that didn't work we could, worst case, have a partial inode
                                                // And that's a "file system is now kinda busted" sort of error
                                                // This is why "real" (read: modern) file systems have backups all over
                                                // (and why the occasional chkdsk is so important)
                                                break;
                                            case FS_DIRECTORY:
                                                // following line keeps being all "Expected expression"
                                                // SOMETHING is messed up SOMEWHERE.
                                                // Or it's trying to protect me by preventing new variables in a switch
                                                // Which is super undefined, but only sometimes (not in this case...)
                                                // Idk, man.
                                                // block_ptr_t new_dir_ptr = back_store_allocate(fs->bs);
                                                new_dir_ptr = back_store_allocate(fs->bs);
                                                if (new_dir_ptr != 0) {
                                                    // Resources = obtained
                                                    // write dir block first, inode is the final step
                                                    // that's more transaction-safe... but it's not like we're thread
                                                    // safe
                                                    // in the slightest (or process safe, for that matter)
                                                    new_inode = (inode_t){
                                                        {0},
                                                        {0, 0777, now, now, now, file_status.inode, FS_DIRECTORY, {0}},
                                                        {new_dir_ptr, 0, 0, 0, 0, 0}};
                                                    strncpy(new_inode.fname, fname_copy, fname_len + 1);

                                                    memset(&new_dir, 0x00, sizeof(dir_block_t));

                                                    if (!(success = full_write(fs, &new_dir, new_dir_ptr)
                                                                    && write_inode(fs, &new_inode, new_inode_idx))) {
                                                        // transation: if it didn't work, release the allocated block
                                                        back_store_release(fs->bs, new_dir_ptr);
                                                    }
                                                }
                                                break;
                                            default:
                                                // HOW.
                                                break;
                                        }
                                        if (success) {
                                            // whoops. forgot the part where I actually save the file to the dir tree
                                            // Mildly important.
                                            unsigned i = 0;
                                            // This is technically a potential infinite loop. But we validated contents
                                            // earlier
                                            for (; parent_dir.entries[i].fname[0] != '\0'; ++i) {
                                            }
                                            strncpy(parent_dir.entries[i].fname, fname_copy, fname_len + 1);
                                            parent_dir.entries[i].inode = new_inode_idx;
                                            ++parent_dir.mdata.size;
                                            if (full_write(fs, &parent_dir, file_status.block)) {
                                                free(path_copy);
                                                return 0;
                                            } else {
                                                // Oh man. These surely are the end times.
                                                // Our file exists. Kinda. But not entirely.
                                                // The final tree link failed.
                                                // We SHOULD:
                                                //  Wipe inode
                                                //  Release dir block (if making a dir)
                                                // But I'm lazy. And if a write failed, why would others work?
                                                // back_store won't actually do that to us, anyway.
                                                // Like, even if the file was deleted while using it, we're mmap'd so
                                                // the kernel has no real way to tell us, as far as I know.
                                                puts("Infinite sadness. New file stuck in limbo.");
                                            }
                                        }
                                    }
                                }
                            }
                        }
                        free(path_copy);
                    }
                }
            }
        }
    }
    return -1;
}

///
/// Opens the specified file for use
///   R/W position is set to the beginning of the file (BOF)
///   Directories cannot be opened
/// \param fs The S16FS containing the file
/// \param path path to the requested file
/// \return file descriptor to the requested file, < 0 on error
///
int fs_open(S16FS_t *fs, const char *path) {
    if(fs && path) {
        result_t res;
        locate_file(fs, path, &res);
        if(res.success && res.found && res.type == FS_REGULAR) {
            size_t fd = bitmap_ffz(fs->fd_table.fd_status);
            if(fd != SIZE_MAX) {
                bitmap_set(fs->fd_table.fd_status, fd);
                fs->fd_table.fd_pos[fd] = 0;
                fs->fd_table.fd_inode[fd] = res.inode;
                return fd;
            }
        } //else bad path
    } //else bad parameter
    return -1;
}

///
/// Closes the given file descriptor
/// \param fs The S16FS containing the file
/// \param fd The file to close
/// \return 0 on success, < 0 on failure
///
int fs_close(S16FS_t *fs, int fd) {
    if(fs && FD_VALID(fd) && bitmap_test(fs->fd_table.fd_status, fd)) {
        bitmap_reset(fs->fd_table.fd_status, fd);
        fs->fd_table.fd_pos[fd] = 0;
        fs->fd_table.fd_inode[fd] = 0;
        return 0;
    } //else bad parameter
    return -1;
}

///
/// Writes data from given buffer to the file linked to the descriptor
///   Writing past EOF extends the file
///   Writing inside a file overwrites existing data
///   R/W position in incremented by the number of bytes written
/// \param fs The S16FS containing the file
/// \param fd The file to write to
/// \param dst The buffer to read from
/// \param nbyte The number of bytes to write
/// \return number of bytes written (< nbyte IFF out of space), < 0 on error
///
int num;
ssize_t fs_write(S16FS_t *fs, int fd, const void *src, size_t nbyte) {
    if(fs && FD_VALID(fd) && bitmap_test(fs->fd_table.fd_status, fd) && src) {
        //let's do this... hopefully... maybe????
        //extract the inode number from fd_table and get the inode
        inode_ptr_t f_inode_ptr = fs->fd_table.fd_inode[fd];
        inode_t f_inode;
        if(read_inode(fs, &f_inode, f_inode_ptr)) {
            size_t position = fs->fd_table.fd_pos[fd]; //postion in file to start writing
            size_t log_block_offset = POSITION_TO_INNER_OFFSET(position); //position offset within logical block
            //cut up nbyte into chunks for logical block, full blocks, and last block
            size_t log_block_writable = BLOCK_SIZE - log_block_offset; //bytes left in logical block
            size_t full_block_writable; //number of full blocks to write
            size_t last_block_writable; //number of leftover bytes to write after logical block and full blocks

            //this is important!
            //if nbyte is bigger than what's available in the first logical block
            if(nbyte > log_block_writable) {
                //we might need more full blocks and/or a partial block at the end
                full_block_writable = (nbyte - log_block_writable) / BLOCK_SIZE;
                last_block_writable = (nbyte - log_block_writable) % BLOCK_SIZE;
//printf("full blocks: %d  last_block: %d  ", full_block_writable + (log_block_writable/BLOCK_SIZE), last_block_writable);
            } else {
                //enough room in logical block to write
                log_block_writable = nbyte;
                full_block_writable = 0;
                last_block_writable = 0;
            }

            //figure out the number of blocks needed for writing
            size_t n_write_blocks = 1 + full_block_writable;
            if(last_block_writable) {
                ++n_write_blocks;
            }

            //initialize array of data block ptrs
            block_ptr_t writable_ptrs[n_write_blocks];
            for(size_t i = 0; i < n_write_blocks; i++) {
                writable_ptrs[i] = 0;
            }
            
            //fill array of block ptrs with existing or newly allocated ptrs
            get_data_block_ptrs(fs, &f_inode, position, n_write_blocks, writable_ptrs);

            //copy src data since we can't do math on void pointers
            // uint8_t src_cpy[nbyte];
            // memcpy(src_cpy, src, nbyte);

            //get data blocks to write to
            uint8_t buffer[n_write_blocks * BLOCK_SIZE];
            for(size_t i = 0; i < n_write_blocks && writable_ptrs[i]; i++) {
                if(!full_read(fs, buffer+i*BLOCK_SIZE, writable_ptrs[i])) {
                    return -1;
                }
            }

            ssize_t bytes_written = 0;

            //loop through all the blocks and copy data
            for(size_t i = 0; i < n_write_blocks && writable_ptrs[i]; i++) {
                if(i == 0) {
                    //special handling for first block to write
                    memcpy(buffer+log_block_offset, src, log_block_writable);
                    bytes_written += log_block_writable;
                } else if(i == n_write_blocks - 1 && last_block_writable) {
                    //special handling for last block to write
                    memcpy(buffer+(i*BLOCK_SIZE), INCREMENT_VOID(src, bytes_written), last_block_writable);
                    bytes_written += last_block_writable;
                } else {
                    //full blocks in the middle
                    memcpy(buffer+(i*BLOCK_SIZE), INCREMENT_VOID(src, bytes_written), BLOCK_SIZE);
                    bytes_written += BLOCK_SIZE;
                }
            }

            //write data blocks back out
            for(size_t i = 0; i < n_write_blocks && writable_ptrs[i]; i++) {
                if(!full_write(fs, buffer+i*BLOCK_SIZE, writable_ptrs[i])) {
                    return -1;
                }
            }

            //update offset in fd_table and file size
            fs->fd_table.fd_pos[fd] += bytes_written;
            if(fs->fd_table.fd_pos[fd] > f_inode.mdata.size) {
                f_inode.mdata.size = fs->fd_table.fd_pos[fd];
            }

            //just need to write out the inode to make sure data_ptrs are updated
            if(write_inode(fs, &f_inode, f_inode_ptr)) {
                
                //if(num >= 500) {printf("my num blocks: %d\n", num);}
                return bytes_written;

            } //else failed to write inode
        } //else failed to read inode
    } //else bad parameter

    return -1;
}

///
/// Deletes the specified file and closes all open descriptors to the file
///   Directories can only be removed when empty
/// \param fs The S16FS containing the file
/// \param path Absolute path to file to remove
/// \return 0 on success, < 0 on error
///
int fs_remove(S16FS_t *fs, const char *path) {
    if(fs && path) {
        result_t file_status;
        locate_file(fs, path, &file_status);
        if(file_status.success && file_status.found && file_status.inode) {
            //we found the file, now get the inode and the parent inode
            inode_t f_inode, parent_inode;
            if(!read_inode(fs, &f_inode, file_status.inode)) {
                return -1;
            }
            if(!read_inode(fs, &parent_inode, file_status.parent)) {
                return -1;
            }
            //if(!read_inode(fs, &f_inode, file_status.inode) || !read_inode(fs, &parent_inode, file_status.parent)) {
            dir_block_t dir;
            size_t f_blocks;
            //do different things based on type
            switch(file_status.type) {
                case FS_REGULAR:
                    //calculate number of data blocks
                    f_blocks = f_inode.mdata.size / BLOCK_SIZE;
                    if(f_inode.mdata.size % BLOCK_SIZE) {
                        ++f_blocks;
                    }
                    //remove all possible occurrences from fd_table
                    for(int i = 0; i < DESCRIPTOR_MAX; i++) {
                        //if the inode number appears in the fd_table, close it 
                        if(bitmap_test(fs->fd_table.fd_status, i) && fs->fd_table.fd_inode[i] == file_status.inode) {
                            fs_close(fs, i);
                        }
                    }
                    break;
                case FS_DIRECTORY:
                    //directories only have one data block
                    if(file_status.type == FS_DIRECTORY) {
                        f_blocks = 1;
                    }
                    //make sure directory is empty
                    if(!full_read(fs, &dir, file_status.block)) {
                        return -1;
                    }
                    if(dir.mdata.size) {
                        return -1;
                    }
                    break;
                default:
                    break;
            }
            
            //don't bother trying to get block pointers if file has no data blocks
            if(f_blocks) {
                block_ptr_t data_ptrs[f_blocks];
                
                get_data_block_ptrs(fs, &f_inode, 0, f_blocks, data_ptrs);
            
                for(size_t i = 0; i < f_blocks && data_ptrs[i]; i++) {
                    back_store_release(fs->bs, data_ptrs[i]);
                }
            }
            
            //clear the inode
            memset(&f_inode, 0x00, sizeof(inode_t));

            //remove fname and inode from parent directory block
            dir_block_t parent_dir;
            if(write_inode(fs, &f_inode, file_status.inode) && full_read(fs, &parent_dir, parent_inode.data_ptrs[0])) {
                for(size_t i = 0; i < DIR_REC_MAX; i++) {
                    if(file_status.inode == parent_dir.entries[i].inode) {
                        parent_dir.entries[i].fname[0] = '\0';
                        parent_dir.entries[i].inode = 0;
                        --parent_dir.mdata.size;
                    }
                }
                if(full_write(fs, &parent_dir, parent_inode.data_ptrs[0])) {
                    return 0;
                } //else failed to write back updated parent directory block
            } //else failed to get parent directory block
            //} //else failed to get file or parent inode
        } //else failed to locate file
    } //else bad parameter
    return -1;
}

///
/// Moves the R/W position of the given descriptor to the given location
///   Files cannot be seeked past EOF or before BOF (beginning of file)
///   Seeking past EOF will seek to EOF, seeking before BOF will seek to BOF
/// \param fs The S16FS containing the file
/// \param fd The descriptor to seek
/// \param offset Desired offset relative to whence
/// \param whence Position from which offset is applied
/// \return offset from BOF, < 0 on error
///
off_t fs_seek(S16FS_t *fs, int fd, off_t offset, seek_t whence);

///
/// Reads data from the file linked to the given descriptor
///   Reading past EOF returns data up to EOF
///   R/W position in incremented by the number of bytes read
/// \param fs The S16FS containing the file
/// \param fd The file to read from
/// \param dst The buffer to write to
/// \param nbyte The number of bytes to read
/// \return number of bytes read (< nbyte IFF read passes EOF), < 0 on error
///
ssize_t fs_read(S16FS_t *fs, int fd, void *dst, size_t nbyte);

///
/// Populates a dyn_array with information about the files in a directory
///   Array contains up to 15 file_record_t structures
/// \param fs The S16FS containing the file
/// \param path Absolute path to the directory to inspect
/// \return dyn_array of file records, NULL on error
///
dyn_array_t *fs_get_dir(S16FS_t *fs, const char *path);

///
/// !!! Graduate Level/Undergrad Bonus !!!
/// !!! Activate tests from the cmake !!!
///
/// Moves the file from one location to the other
///   Moving files does not affect open descriptors
/// \param fs The S16FS containing the file
/// \param src Absolute path of the file to move
/// \param dst Absolute path to move the file to
/// \return 0 on success, < 0 on error
///
int fs_move(S16FS_t *fs, const char *src, const char *dst);

///
/// Fills array of block_ptr_t with ptrs to data blocks requested
///     write can request blocks that don't exist, therefore block allocation will occur
///     remove gets all the block ptrs already allocated to a file
///     read will get all block ptrs requested up to EOF, no allocation
/// \param fs - The S16FS containing the file
/// \param f_inode - pointer to the file's inode in memory
/// \param position - byte offset in file to start getting blocks
///     write uses position from fd_table
///     remove uses 0
///     read will use position from fd_table
/// \param n_blocks - number of blocks to get
///     write only needs enough blocks to write requested number of bytes
///     remove needs all the blocks
///     read will only need enough blocks to read requested number of bytes
/// \param ptrs - block_ptr_t array to be filled
///
void get_data_block_ptrs(S16FS_t *fs, inode_t *f_inode, size_t position, size_t n_blocks, block_ptr_t *ptrs) {
    size_t log_block_index = POSITION_TO_BLOCK_INDEX(position); //logical index for first block requested
    size_t j = 0; //for ptrs array indexing
    size_t i = log_block_index; //logical file block indexing
    bool good = true;

    //get direct ptrs
    while(i < DIRECT_TOTAL && j < n_blocks && good) {
        //check the ith direct ptr
        if(!(f_inode->data_ptrs[i])) {
            //request new direct block and validate
            ++(num);
            f_inode->data_ptrs[i] = back_store_allocate(fs->bs);
            if(!(f_inode->data_ptrs[i])) {
                good = false;
            }
        }
        if(good) {
            //either we had a ptr already or allocation was successful
            //get data block ptr and increment indices
            ptrs[j] = f_inode->data_ptrs[i];
            ++j;
            ++i;
        }
    }

    //get single indirect data block ptrs if requested
    if(i < (DIRECT_TOTAL + INDIRECT_TOTAL) && j < n_blocks && good) {
        block_ptr_t i_block[INDIRECT_TOTAL] = {0};
        //check indirect ptr
        if(!(f_inode->data_ptrs[6])) {
            //request new indirect block and validate
            f_inode->data_ptrs[6] = back_store_allocate(fs->bs);
            ++(num);
            if(!(f_inode->data_ptrs[6])) {
                good = false;
            }
        } else {
            //get existing indirect block
            if(!full_read(fs, i_block, f_inode->data_ptrs[6])) {
                good = false;
            }
        }
        if(good) {
            //we have an indirect block in i_block, either new or existing
            //loop to get data blocks
            size_t h = (i-DIRECT_TOTAL) % INDIRECT_TOTAL;
            for(; h < INDIRECT_TOTAL && j < n_blocks && good; h++) {
                //check if we need to allocate a block
                if(!i_block[h]) {
                    //request new data block and validate
                    i_block[h] = back_store_allocate(fs->bs);
                    //printf("new indirect -> data %d\n", h);
                    ++(num);
                    if(!i_block[h]) {
                        good = false;
                    }
                }
                if(good) {
                    //all is well, get data block ptr
                    ptrs[j] = i_block[h];
                    ++j;
                    ++i;
                }
            }
            //write out the i_block to save any changes
            if(!full_write(fs, i_block, f_inode->data_ptrs[6])) {
                good = false;
            }
        }
    }

    //get double indirect data blocks ptrs if requested
    if(j < n_blocks && good) {
        block_ptr_t d_block[INDIRECT_TOTAL] = {0};
        //check dbl indirect ptr
        if(!(f_inode->data_ptrs[7])) {
            ++(num);
            //request new double indirect block
            f_inode->data_ptrs[7] = back_store_allocate(fs->bs);
            if(!(f_inode->data_ptrs[7])) {
                good = false;
            }
        } else {
            //get existing double indirect block
            if(!full_read(fs, d_block, f_inode->data_ptrs[7])) {
                good = false;
            }
        }
        if(good) {
            //now we do the same damn thing as for single indirect.. 
            //maybe this should be modularized, but whatever
            //outer loop gets indirect blocks
            size_t k = (i-(DIRECT_TOTAL + INDIRECT_TOTAL)) / INDIRECT_TOTAL;
            for(; j < n_blocks && good && k < INDIRECT_TOTAL; k++) {

                block_ptr_t i_block[INDIRECT_TOTAL] = {0};
                //check if we need to allocate kth indirect block
                if(!d_block[k]) {
                    //request new indirect block
                    d_block[k] = back_store_allocate(fs->bs);
                    ++(num);
                    if(!d_block[k]) {
                        good = false;
                    }
                } else {
                    //get kth indirect block
                    if(!full_read(fs, i_block, d_block[k])) {
                        good = false;
                    }
                }
                if(good) {
                    //inner loop to get data blocks from kth indirect block
                    size_t h = (i-(DIRECT_TOTAL + INDIRECT_TOTAL)) % INDIRECT_TOTAL;
                    for(; j < n_blocks && good && h < INDIRECT_TOTAL; h++) {
                        //check if we need to allocate hth data block
                        if(!i_block[h]) {
                            //request new data block and validate
                            i_block[h] = back_store_allocate(fs->bs);
                            ++(num);
                            if(!i_block[h]) {
                                good = false;
                            }
                        }
                        //get data block ptr
                        if(good) {
                            ptrs[j] = i_block[h];
                            ++j;
                            ++i;
                        }
                    }
                    //write kth indirect block back out to save any changes
                    if(!full_write(fs, i_block, d_block[k])) {
                        good = false;
                    }
                }
            }
        }
        //write double indirect block back out to save any changes
        if(!full_write(fs, d_block, f_inode->data_ptrs[7])) {
            good = false;
        }
    }

    //if anything went wrong, ptrs array has 0's
    //calling functions need to check all the ptr they try to use
    return;
}

/*
void print_file(S16FS_t *fs, inode_t *f_inode) {
    size_t f_blocks = f_inode->mdata.size / BLOCK_SIZE;
    if(f_inode->mdata.size % BLOCK_SIZE) {
        ++f_blocks;
    }

    block_ptr_t block_ptrs[f_blocks];
    get_data_block_ptrs(fs, f_inode, 0, f_blocks, block_ptrs);

    char data[f_blocks*BLOCK_SIZE];

    for(size_t i = 0; i < f_blocks; i++) {
        full_read(fs, data+(i*BLOCK_SIZE), block_ptrs[i]);
    }

    int fd = open(f_inode->fname, O_WRONLY | O_CREAT | O_TRUNC, S_IRUSR | S_IWUSR | S_IRGRP | S_IWGRP);
    write(fd, data, f_blocks*BLOCK_SIZE);
    close(fd);

    return;
}
*/