#ifndef _BACKEND_H__
#define _BACKEND_H__

#include "S16FS.h"

#include <back_store.h>
#include <bitmap.h>

#include <stdint.h>
#include <sys/types.h>

// typedef enum { FS_SEEK_SET, FS_SEEK_CUR, FS_SEEK_END } seek_t;

// typedef enum { FS_REGULAR, FS_DIRECTORY } file_t;

#define FS_FNAME_MAX (64)
// INCLUDING null terminator

#define DIR_REC_MAX (15)
// can't have more than 15 records in a directory because space
// And you're limited to one directory block because we're nice :)

// It's actually... 16321 + 1 since we run out of inodes
// (that's 63 * 255 + 256 + 1 = 255 (additional) dirs, begin, end, intermediary slashes, and null terminator)
#define FS_PATH_MAX (16322)

#define DESCRIPTOR_MAX (256)

#define BLOCK_SIZE (1024)

#define INODE_BLOCK_TOTAL (32)

#define INODES_PER_BOCK (BLOCK_SIZE / sizeof(inode_t))

// checking inode numbers with this is a tautology for inode_ptr_t :/
#define INODE_TOTAL (((INODE_BLOCK_TOTAL) * (BLOCK_SIZE)) / sizeof(inode_t))

#define INODE_BLOCK_OFFSET (8)

#define DATA_BLOCK_OFFSET ((INODE_BLOCK_OFFSET) + (INODE_BLOCK_TOTAL))

// It's always going to be here, so I might as well macro it because locate_file has a potential disaster
// since I added block to result_t and I preload the results with the info for root for nice looping
// If you actually search for root, it'll eat it the way it is now. Fixed with this.
#define ROOT_DIR_BLOCK (DATA_BLOCK_OFFSET)

#define INODE_PTR_TOTAL (8)

#define DIRECT_TOTAL (6)

#define INDIRECT_TOTAL ((BLOCK_SIZE) / sizeof(block_ptr_t))

#define DBL_INDIRECT_TOTAL ((INDIRECT_TOTAL) * (INDIRECT_TOTAL))

#define FILE_SIZE_MAX ((DIRECT_TOTAL + INDIRECT_TOTAL + DBL_INDIRECT_TOTAL) * BLOCK_SIZE)

#define DATA_BLOCK_MAX (65536)

// Calcs what block an inode is in
#define INODE_TO_BLOCK(inode) (((inode) >> 3) + INODE_BLOCK_OFFSET)

// Calcs the index an inode is at within a block
#define INODE_INNER_IDX(inode) ((inode) &0x07)

#define INODE_INNER_OFFSET(inode) (INODE_INNER_IDX(inode) * sizeof(inode_t))

// Converts a file position to a block index (note: not a block id. index 6 is the 6th block of the file)
#define POSITION_TO_BLOCK_INDEX(position) ((position) >> 10)

// Position within a block
#define POSITION_TO_INNER_OFFSET(position) ((position) &0x3FF)

// tells you if the given block index makes sense
// but results in a tautology if you're using block_ptr_t :/
#define BLOCK_IDX_VALID(block_idx) ((block_idx) >= DATA_BLOCK_OFFSET && (block_idx) < DATA_BLOCK_MAX)
// Well it avoids the tautology... btu I don't like it.
#define BLOCK_PTR_VALID(block_idx) ((block_idx) >= DATA_BLOCK_OFFSET)

// Checks that an inode is the specified type
#define INODE_IS_TYPE(inode_ptr, file_type) ((inode_ptr)->mdata.type & (file_type))

// Because you can't increment an invalid type
// And writing out the cast every time gets tiring
// Also works when the type just isn't what you want
#define INCREMENT_VOID(v_ptr, increment) (((uint8_t *) (v_ptr)) + (increment))

typedef uint8_t data_block_t[BLOCK_SIZE];  // c is weird
typedef uint16_t block_ptr_t;
typedef uint8_t inode_ptr_t;

typedef struct {
    uint32_t size;    // Probably all I'll use for directory file metadata
    uint32_t mode;    // not used in the slightest
    uint32_t c_time;  // creation (technically not what c_time is, but I like this better)
    uint32_t a_time;  // access
    uint32_t m_time;  // modification
    // None of these will probably be used
    inode_ptr_t parent;  // SO NICE TO HAVE. You'll be so mad if you didn't think of it, too
    uint8_t type;
    // Would be nice to just use the actual enum, but that's not going to go well
    // Figuring out why is an excercise for the reader (or just ask...)

    // And, uhh, 26 bytes left and I'm already probably going
    // to forget to update the times appropriately
    uint8_t padding[26];
} mdata_t;


typedef struct {
    char fname[FS_FNAME_MAX];
    mdata_t mdata;
    block_ptr_t data_ptrs[8];
} inode_t;

typedef struct {
    char fname[FS_FNAME_MAX];
    inode_ptr_t inode;
} dir_ent_t;

typedef struct {
    mdata_t mdata;
    dir_ent_t entries[DIR_REC_MAX];
    uint8_t padding;  // SO CLOSE, but there was one left over byte.
} dir_block_t;

typedef struct {
    bitmap_t *fd_status;
    size_t fd_pos[DESCRIPTOR_MAX];
    inode_ptr_t fd_inode[DESCRIPTOR_MAX];
} fd_table_t;

struct S16FS {
    back_store_t *bs;
    fd_table_t fd_table;
};

typedef struct { block_ptr_t block_ptrs[INDIRECT_TOTAL]; } indir_block_t;

/*
typedef struct {
    // You can add more if you want
    // vvv just don't remove or rename these vvv
    char[FS_FNAME_MAX] name;
    file_t type;
} file_record_t;
*/

// completely generic structure with a smattering of types
// functions that use it say what they fill out
typedef struct {
    bool success, found, valid;
    inode_ptr_t inode, parent;
    block_ptr_t block;
    file_t type;
    uint64_t total;
    uint64_t pos;
    void *data;
} result_t;

typedef enum { OVERWRITE = 0x01, EXTEND = 0x02, MIXED = 0x03 } write_mode_t;

#define GET_WRITE_MODE(file_size, position, nbytes) \
    (((position) < (file_size)) ? ((((position) + (nbytes)) <= file_size) ? OVERWRITE : MIXED) : EXTEND)

#ifdef DEBUG

#define DBG_PRINT_SETUP() bool dbg_print_flag = true
#define DBG_PRINT(print_str, ...) printf("[!] %s:%d: " print_str "\n", __FILE__, __LINE__, ##__VA_ARGS__)
#define DBG_PRINT_ONCE(print_str, ...)       \
    if (dbg_print_flag) {                    \
        DBG_PRINT(print_str, ##__VA_ARGS__); \
        dbg_print_flag = false;              \
    }

#else

#define DBG_PRINT_SETUP()
#define DBG_PRINT(print_str, ...)
#define DBG_PRINT_ONCE(print_str, ...)

#endif

bool partial_read(const S16FS_t *fs, void *data, const block_ptr_t block, const unsigned offset, const unsigned bytes);
bool partial_write(S16FS_t *fs, const void *data, const block_ptr_t block, const unsigned offset, const unsigned bytes);
bool full_read(const S16FS_t *fs, void *data, const block_ptr_t block);
bool full_write(S16FS_t *fs, const void *data, const block_ptr_t block);
bool read_inode(const S16FS_t *fs, void *data, const inode_ptr_t inode_number);
bool write_inode(S16FS_t *fs, const void *data, const inode_ptr_t inode_number);
bool clear_inode(S16FS_t *fs, const inode_ptr_t inode_number);

void locate_file(const S16FS_t *const fs, const char *abs_path, result_t *res);
void scan_directory(const S16FS_t *const fs, const char *fname, const inode_ptr_t inode, result_t *res);

inode_ptr_t find_free_inode(const S16FS_t *const fs);

S16FS_t *ready_file(const char *path, const bool format);

bool wipe_parent_entry(S16FS_t *const fs, const char *fname, const inode_ptr_t parent_id);
bool release_regular(S16FS_t *fs, const char *fname, const inode_ptr_t target_number, const inode_ptr_t parent_number);
bool release_dir(S16FS_t *fs, const char *fname, const inode_ptr_t target_number, const inode_ptr_t parent_number,
                 const block_ptr_t block_number);

bool fd_valid(const S16FS_t *const fs, int fd);

ssize_t overwrite_file(S16FS_t *fs, const inode_t *inode, const size_t position, const void *data, size_t nbyte);
ssize_t extend_file(S16FS_t *fs, inode_t *inode, inode_ptr_t inode_number, size_t new_len);

dyn_array_t *get_blocks(const S16FS_t *fs, const inode_t *inode, const size_t first, const size_t last);

void release_fds(S16FS_t *fs, int inode_number);

#endif
