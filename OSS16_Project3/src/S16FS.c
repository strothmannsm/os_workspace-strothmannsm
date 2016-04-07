#include "../include/S16FS.h"
#include <back_store.h>
#include <bitmap.h>
#include <string.h>
#include <stdio.h>
#include <stdlib.h>
#include <time.h>

#define INODE_TABLE_BYTES 32768
#define N_INODES 256
#define N_INODE_BLOCKS 32
#define DIR_REC_MAX 15
#define FS_FD_MAX 256
#define INODES_PER_BLOCK 8
#define FBM_BLOCKS 8
#define BLOCK_SIZE 1024

typedef uint8_t inode_ptr_t;
typedef uint16_t block_ptr_t;

typedef struct {
	uint32_t	size;			//size of file in bytes
	uint32_t	i_ctime;		//file creation time
	uint16_t	blocks;			//size of file in blocks
	inode_ptr_t	i_ino;			//file inode index
	inode_ptr_t	dir_ino;		//parent directory inode index
	uint8_t		padding[32];	//padding for size
	//what else...
	//since type is 4 bytes inside file_record_t mdata_t is only 44 bytes
} mdata_t;

typedef struct {
	file_record_t	file;		//file name and type see struct def in header
	mdata_t			mdata;		//other file metadata
	block_ptr_t		blocks[8];	//block pointers, 6 direct, 1 indirect, 1 double indirect
} inode_t;

typedef struct {
	char			name[FS_FNAME_MAX]; //file name
	inode_ptr_t		inode;				//file inode index
} dir_entry_t;

typedef struct {
	mdata_t			mdata;				  //the inode metadata copied to the block?
	dir_entry_t		entries[DIR_REC_MAX]; //directory entries
	uint8_t			n_entries;			  //use a byte just to track the number of entries in the directory
	uint8_t			padding[4];			  //mdata_t is only 44 bytes so still need 4 padding bytes
} dir_block_t;

typedef struct {
	bitmap_t		*fd_status;			 //bitmap to track validity of file descriptors
	size_t			fd_pos[FS_FD_MAX];	 //offsets for file descriptors
	inode_ptr_t		fd_inode[FS_FD_MAX]; //inode indexes for file descriptors
} fd_table_t;

struct S16FS {
	back_store_t	*bs;				//back_store object (logical block device)
	inode_t			i_table[N_INODES];	//inode table (saved to and read from block device)
	fd_table_t		fd_table;			//file descriptor table
};

/*
 *	helper functions: pretty self explanatory,
 *	but see implementations at bottom for more info
 */
bool write_inode_block(S16FS_t *fs, inode_ptr_t i_block);
bool init_dir_block(S16FS_t *fs, inode_ptr_t dir_idx);
bool valid_path(const char *path);
dyn_array_t *parse_path(const char *path);
int traverse_path(S16FS_t *fs, dyn_array_t *parsed);

///
/// Formats (and mounts) an S16FS file for use
/// \param fname The file to format
/// \return Mounted S16FS object, NULL on error
///
S16FS_t *fs_format(const char *fname) {
	if(fname && strcmp("", fname)) {
		//validate fname more?
		//allocate S16FS_t object and validate
		S16FS_t *fs = (S16FS_t*)calloc(1, sizeof(S16FS_t));
		if(fs) {
			//create back_store (this is our logical block device) and validate
			fs->bs = back_store_create(fname);
			if(fs->bs) {
				//get bitmap for fd_table tracking and validate
				fs->fd_table.fd_status = bitmap_create(FS_FD_MAX);
				if(fs->fd_table.fd_status) {
					//request && write blocks for inode table
					bool valid = true;
					for(block_ptr_t i = 0; i < N_INODE_BLOCKS && valid; i++) {
						if(!back_store_request(fs->bs, i+FBM_BLOCKS) || !write_inode_block(fs, i)) {
							//failed to request or write a block for the inode table
							valid = false;
						}
					}

					if(valid) {
						//fill in root directory inode
						strcpy(fs->i_table[0].file.name, "root");
						fs->i_table[0].file.type = FS_DIRECTORY;
						fs->i_table[0].mdata.i_ctime = time(NULL);
						//initialize root directory block && write out updated inode to device
						if(init_dir_block(fs, 0) && write_inode_block(fs, 0)) {
							return fs;
						} //else failed to initialize root directory block
					} // else failed to request inode table block
				} //else failed to create fd_table.fd_status bitmap
			} //else failed to create back_store
			free(fs);
		} //else failed to allocate S16FS_t object
	} //else bad parameter
	return NULL;
}

///
/// Mounts an S16FS object and prepares it for use
/// \param fname The file to mount
/// \return Mounted F16FS object, NULL on error
///
S16FS_t *fs_mount(const char *fname) {
	if(fname && strcmp("", fname)) {
		//validate file name some more?
		//allocate S16FS_t object and validate
		S16FS_t *fs = (S16FS_t*)calloc(1, sizeof(S16FS_t));
		if(fs) {
			//open back_store (logical block device) and validate
			fs->bs = back_store_open(fname);
			if(fs->bs) {
				//get bitmap for fd_table tracking and validate
				fs->fd_table.fd_status = bitmap_create(FS_FD_MAX);
				if(fs->fd_table.fd_status) {
					// load inode table
					bool valid = true;
					for(unsigned i = 0; i < N_INODE_BLOCKS && valid; i++) {
						if(!back_store_read(fs->bs, i+FBM_BLOCKS, &fs->i_table[i*INODES_PER_BLOCK])) {
							//failed to read an inode table block
							valid = false;
						}
					}
					//make sure we read all the inode table blocks
					if(valid) {
						return fs;
					} //else failed to read an inode table block
				} //else failed to create fd_table.fd_status bitmap
			} //else failed to open the back_store
			free(fs);
		} //else failed to allocate S16FS_t object
	} //else bad parameter
	return NULL;
}

///
/// Unmounts the given object and frees all related resources
/// \param fs The S16FS object to unmount
/// \return 0 on success, < 0 on failure
///
int fs_unmount(S16FS_t *fs) {
	if(fs) {
//close any open file descriptors
		
		//save entire inode table
		bool valid = true;
		for(unsigned i = 0; i < N_INODE_BLOCKS && valid; i++) {
			if(!write_inode_block(fs, i)) {
				valid = false;
			}
		}
		//make sure we wrote out all the inode blocks
		if(valid) {
			//clean up S16FS_t object
			back_store_close(fs->bs);
			bitmap_destroy(fs->fd_table.fd_status);
			free(fs);
			return 0;
		} //else failed to write an inode table block
	} //else bad parameter
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
	if(fs && path && valid_path(path) && type <= FS_DIRECTORY) {
		//parse the path into a series of elements
		dyn_array_t *parsed = parse_path(path);
		if(parsed) {
			//get the name of the new file back (last path element)
			char fname[FS_FNAME_MAX];
			if(dyn_array_extract_back(parsed, fname)) {
				//get the inode index for the last path element
				int dir_idx = traverse_path(fs, parsed);
				//inode_ptr_t
				//done with dyn_array
				dyn_array_destroy(parsed);
				//make sure we found a target location from traverse_path
				if(dir_idx != -1) {
					//get the inode
					inode_t *dir_inode = &fs->i_table[dir_idx];
					//make sure the destination is a directory
					//if(dir_inode->mdata.type == FS_DIRECTORY)
					if(dir_inode->file.type == FS_DIRECTORY) {
						//get the destination directory block
						dir_block_t dir_block;
						if(back_store_read(fs->bs, dir_inode->blocks[0], &dir_block)) {
							//make sure the destination directory isn't full
							if(dir_block.n_entries < DIR_REC_MAX) {
								//make sure fname doesn't already exists in the destination directory
								for(int i = 0; i < dir_block.n_entries; i++) {
									if(!strcmp(fname, dir_block.entries[i].name)) {
										return -1;
									}
								}
								//search for open inode
								inode_ptr_t f_idx = 0;
								for(unsigned i = 1; !f_idx && i < N_INODES; i++) {
									if(fs->i_table[i].mdata.i_ctime == 0) {
										//found open inode
										f_idx = (inode_ptr_t)i;
									} 
								}
								//make sure we found an open inode
								if(f_idx) {
									inode_t *f_node = &fs->i_table[f_idx];
									//fill new inode
									strcpy(f_node->file.name, fname);
									f_node->file.type = type;
									f_node->mdata.i_ino = f_idx;
									f_node->mdata.dir_ino = (inode_ptr_t)dir_idx;
									f_node->mdata.i_ctime = time(NULL);
									if(write_inode_block(fs, f_idx/INODES_PER_BLOCK)) {
										//update destination directory block
										strcpy(dir_block.entries[dir_block.n_entries].name, fname);
										dir_block.entries[dir_block.n_entries].inode = f_idx;
										dir_block.n_entries++;
										//write destination directory block back out
										if(back_store_write(fs->bs, dir_inode->blocks[0], &dir_block)) {
											//last little bit of work depending on file type created
											if(type == FS_REGULAR) {
												//regular file, we're done
												return 0;
												//type already validated so else is sufficient, but just for clarity
											} else if(type == FS_DIRECTORY) {
												f_node->mdata.blocks = 1;
												//initialize directory block
												if(init_dir_block(fs, f_idx)) {
													//success, we're done
													return 0;
												} //else failed to init directory block
											} //else nothing, type had to be FS_REGULAR or FS_DIRECTORY to get here
										} //else failed to write updated directory block
									} //else failed to write updated inode to device
								} //else no open inodes
							} //else directory is full
						} //else back_store_read failed
					} //else target location is not a directory
				} //else failed to traverse path
			} //else failed to extract fname from parsed
		} //else failed to parse path
	} //else bad parameter
	return -1;
}
/*
///
/// Opens the specified file for use
///   R/W position is set to the beginning of the file (BOF)
///   Directories cannot be opened
/// \param fs The S16FS containing the file
/// \param path path to the requested file
/// \return file descriptor to the requested file, < 0 on error
///
int fs_open(S16FS_t *fs, const char *path) {
	if(fs && path && valid_path(path)) {
		dyn_array_t *parsed = parse_path(path);
		if(parsed) {
			int f_idx = traverse_path(fs, parsed);
			dyn_array_destroy(parsed);
			if(f_idx != -1) {
				//get the inode and make sure it's a regular file
				inode_t *f_inode = &fs->i_table[f_idx];
				if(f_inode->file.type == FS_REGULAR) {
					//find an available file descriptor
					unsigned int fd = bitmap_ffz(fs->fd_table.fd_status);
					if(fd != SIZE_MAX) {
						//initialize file descriptor
						bitmap_set(fs->fd_table.fd_status, fd);
						fs->fd_table.fd_pos[fd] = 0;
						fs->fd_table.fd_inode[fd] = (inode_ptr_t)f_idx;
						return (int)fd;
					} //else maxed out on open files
				} //else target is a directory
			} //else failed to traverse path
		} //else failed to parse path
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
	if(fs && bitmap_test(fs->fd_table.fd_status, fd)) {
		//just have to invalidate the file descriptor to close the file
		bitmap_reset(fs->fd_table.fd_status, fd);
		return 0;
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
//off_t fs_seek(S16FS_t *fs, int fd, off_t offset, seek_t whence);

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
//ssize_t fs_read(S16FS_t *fs, int fd, void *dst, size_t nbyte);

///
/// Writes data from given buffer to the file linked to the descriptor
///   Writing past EOF extends the file
///   Writing inside a file overwrites existing data
///   R/W position is incremented by the number of bytes written
/// \param fs The S16FS containing the file
/// \param fd The file to write to
/// \param src The buffer to write from
/// \param nbyte The number of bytes to write
/// \return number of bytes written (< nbyte IFF out of space), < 0 on error
///
ssize_t fs_write(S16FS_t *fs, int fd, const void *src, size_t nbyte) {
	if(fs && bitmap_test(fs->fd_table.fd_status, fd) && src && nbyte) {

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
	if(fs && path && valid_path(path)) {
		dyn_array_t *parsed = parse_path(path);
		if(parsed) {
			int f_idx = traverse_path(fs, parsed);
			dyn_array_destroy(parsed);
			if(f_idx != -1) {
				//get the inode, the parent directory inode and directory block
				inode_t *f_inode = &fs->i_table[f_idx];
				inode_t *dir_inode = &fs->i_table[f_inode->mdata.dir_ino];
				dir_block_t dir_block;
				if(back_store_read(fs->bs, dir_inode->blocks[0], &dir_block)) {
					bool valid = true;
					//if regular file, make sure all FDs closed
					if(f_inode->file.type == FS_REGULAR) {
						//loop through fd_table
						for(int i = 0; i < FS_FD_MAX; i++) {
							//if fd is valid and points to this file, close it
							if(bitmap_test(fs->fd_table.fd_status, i) && (fs->fd_table.fd_inode[i] == (inode_ptr_t)f_idx)) {
								fs_close(fs, i);
							}
						}
					} else { //else directory, make sure it's empty
						//get target directory block and make sure it's empty
						dir_block_t target;
						if(!back_store_read(fs->bs, f_inode->blocks[0], &target) || (target.n_entries != 0)) {
							valid = false;
						}
					}

					if(valid) {

						if(f_inode->mdata.blocks > 6) {
							//get indirect blocks
						}
						if(f_inode->mdata.blocks > 518) {
							//get double indirect blocks
						}
						for(int i = 0; i < f_inode->mdata.blocks; i++) {

						}

						//clear f_inode
						memset(f_inode, 0x00, sizeof(inode_t));
				//update dir_block and write out
				//remove entry and --n_entries
						dir_block.n_entries--;
						if(back_store_write(fs->bs, dir_inode->blocks[0], &dir_block)) {
							//done now?
							return 0;
						} //else failed to write out parent dir_block
					} //else target directory is not empty
				} //else failed to read parent dir_block
			} //else failed to traverse path
		} //else failed to parse path
	} //else bad parameter
	return -1;
}

///
/// Populates a dyn_array with information about the files in a directory
///   Array contains up to 15 file_record_t structures
/// \param fs The S16FS containing the file
/// \param path Absolute path to the directory to inspect
/// \return dyn_array of file records, NULL on error
///
//dyn_array_t *fs_get_dir(S16FS_t *fs, const char *path);

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
//int fs_move(S16FS_t *fs, const char *src, const char *dst);
*/
/*
 * writes the requested block of the inode table to the device
 * PARAMETERS: fs - the file system, i_block - logical index of inode table block (0-31)
 * RETURNS: bool for success/failure
 */
bool write_inode_block(S16FS_t *fs, inode_ptr_t i_block) {
	if(fs) {
		//attempt to write out the requested inode table block
		if(back_store_write(fs->bs, i_block + FBM_BLOCKS, &fs->i_table[i_block * INODES_PER_BLOCK])) {
			return true;
		} //else failed to write i_table block to device
	} //else bad parameter
	return false;
}

/*
 * initializes a directory block (root directory for format or creating new directory)
 * PARAMETERS: fs - the file system, dir_idx - the inode index of the directory to create a block for
 * RETURNS: bool for success/failure
 */
bool init_dir_block(S16FS_t *fs, inode_ptr_t dir_idx) {
	// get the inode for the directory and allocate a block for the dir_block
	inode_t *dir_inode = &fs->i_table[dir_idx];
	dir_inode->blocks[0] = back_store_allocate(fs->bs);
	//did we get a block successfully? && did we write out the updated inode?
	if(dir_inode->blocks[0] && write_inode_block(fs, dir_idx/INODES_PER_BLOCK)) {
		//calloc a dir_block to start with clean block
		dir_block_t *dir_block = (dir_block_t*)calloc(1, sizeof(dir_block_t));
		if(dir_block) {
//copy dir_inode->mdata to dir_block? why?
			//attempt to write the dir_block out
			if(back_store_write(fs->bs, dir_inode->blocks[0], dir_block)) {
				free(dir_block);
				return true;
			} //else failed to write out directory block
			free(dir_block);
		} //else failed to calloc a new directory block
	} //else failed to allocate block for directory or failed to write i_table block to device

	return false;
}

/*
 * validates a given absolute path
 * PARAMETERS: path - string to validate
 * RETURNS: bool for valid/invalid
 */
bool valid_path(const char *path) {
	// rather than manually checking all of these every time we get a path
	// just pass it down here for validation
	if(strcmp("", path) && path[0] == '/' && path[strlen(path)-1] != '/') {
		//more path validation than this?
		return true;
	}

	return false;
}

/*
 * parses path into dyn_array of individual elements
 * PARAMETERS: path - absolute path to parse
 * RETURNS: dyn_array_t* of parsed path elements or NULL on error
 */
dyn_array_t *parse_path(const char *path) {
	char *path_copy = strdup(path); //copy path for parsing
	if(path_copy) {
		dyn_array_t *parsed = dyn_array_create(0, FS_FNAME_MAX, NULL);
		if(parsed) {
			//get the first path element
			char *token = strtok(path_copy, "/");
			bool valid = true;
			while(token && valid) {
				if(strlen(token) < FS_FNAME_MAX) {
					//path element is valid length filename
					//copy into "full-size" name for dyn_array
					char elem[FS_FNAME_MAX];
					strcpy(elem, token);
					if(!dyn_array_push_back(parsed, elem)) {
						//failed to push the element onto the dyn_array
						valid = false;
					}
					//get the next path element
					token = strtok(NULL, "/");
				} else {
					//path element given is not valid length
					valid = false;
				}
			}
			//either we've finished parsing or something bad happened - let's check
			if(valid && !dyn_array_empty(parsed)) {
				//nothing bad happened and we actually have something in the dyn_array
				free(path_copy); //clean up memory - done below if something went wrong
				return parsed;
			} //else something went wrong
			dyn_array_destroy(parsed);
		} //else couldn't create a dyn_array
		free(path_copy);
	} //else couldn't copy the path for parsing
	return NULL;
}

/*
 * traverses a directory path to the terminal node
 * calling functions must error check valid returns to make sure it's what they wanted
 * i.e file type or when root is returned
 * PARAMETERS: fs - the file system, parsed - dyn_array of path elements
 * RETURNS: inode index of terminal node, -1 on error
 */
int traverse_path(S16FS_t *fs, dyn_array_t *parsed) {
	if(fs && parsed) {
		if(!dyn_array_empty(parsed)) {
			//get the root inode and root directory block
			inode_t *cur_node = &fs->i_table[0];
			dir_block_t dir_block;
			if(back_store_read(fs->bs, cur_node->blocks[0], &dir_block)) {
				//traverse the path
				while(!dyn_array_empty(parsed)) {
					//get the path element
					char elem[FS_FNAME_MAX];
					if(dyn_array_extract_front(parsed, elem)) {
						//look for path element in this directory
						bool found = false;
						inode_ptr_t idx;
						for(int i = 0; !found && i < dir_block.n_entries; i++) {
							if(!strcmp(elem, dir_block.entries[i].name)) {
								//found the path element
								found = true;
								idx = dir_block.entries[i].inode;
							} //else not found keep looking
						}

						if(found) {
							//get the inode for this path element
							cur_node = &fs->i_table[idx];
						} else {
							//path element doesn't exist in the current directory
							return -1;
						}

						if(dyn_array_empty(parsed)) {
							//found a terminal node, calling function must check if it's the right type
							return idx;
						} else if(cur_node->file.type == FS_REGULAR) {
							//non-terminal node is not a directory
							return -1;
						} else if(cur_node->file.type == FS_DIRECTORY) {
							//non-terminal node is another directory
							//get directory block and keep traversing
							if(!back_store_read(fs->bs, cur_node->blocks[0], &dir_block)) {
								//failed to get new directory block
								return -1;
							}
						} else {
							//what happened? ABORT!!!
							return -1;
						}
					} //else failed to extract element from dyn_array
				} //endwhile
			} //else failed to read the root directory block
		} else { //the dyn_array is empty
			//coming from fs_create that means the target is the root
			return 0;
		}
	} //else bad parameter (shouldn't happen since parameter checking done in calling functions)
	return -1;
}