#include <stdio.h>
#include <stdlib.h>
#include <fcntl.h>
#include <string.h>
#include <stdbool.h>
#include <unistd.h>
#include <errno.h>
#include <bitmap.h>
#include "../include/back_store.h"

#define BLOCK_SIZE 1024 //2^10 bytes = 1 KB blocks
#define NUM_BLOCKS 65536 //2^16 blocks
#define FBM_BLOCKS 8 //NUM_BLOCKSbits / (8bits * BLOCK_SIZEbytes) = 8 blocks for FBM data
#define FBM_BYTES 8192 //FBM_BLOCKS * BLOCK_SIZE

struct back_store {
    int fd; // file descriptor for backing store
    bitmap_t *fbm; // bitmap for free block map
};

///
/// Creates a new back_store file at the specified location
///  and returns a back_store object linked to it
/// \param fname the file to create
/// \return a pointer to the new object, NULL on error
///
back_store_t *back_store_create(const char *const fname) {

    if(!fname || !strcmp(fname, "\0") || !strcmp(fname, "\n")) {
        return NULL;
    }

    //create back_store_t object in heap memory
    back_store_t *bs = (back_store_t*)malloc(sizeof(back_store_t));

    //open the file and store the file descriptor
    int flags = O_CREAT | O_TRUNC | O_RDWR;
    int mode = S_IRUSR | S_IWUSR | S_IRGRP | S_IWGRP;
    int fd = open(fname, flags, mode);
    if(fd == -1) {
        free(bs);
        return NULL;
    }
    bs->fd = fd;

    //create a new bitmap
    bs->fbm = bitmap_create(NUM_BLOCKS);
    if(bs->fbm == NULL) {
        close(bs->fd);
        free(bs);
        return NULL;
    }

    //set first bits in bitmap to reserve blocks for FBM
    for(unsigned i = 0; i < FBM_BLOCKS; i++) {
        bitmap_set(bs->fbm, i);
    }

    //initialize all blocks
    uint8_t block[BLOCK_SIZE];
    memset(block, 0x00, BLOCK_SIZE);
    for(unsigned i = 0; i < NUM_BLOCKS; i++) {
        write(bs->fd, block, BLOCK_SIZE);
    }

    return bs;
}

///
/// Opens the specified back_store file
///  and returns a back_store object linked to it
/// \param fname the file to open
/// \return a pointer to the new object, NULL on error
///
back_store_t *back_store_open(const char *const fname) {

    if(!fname || !strcmp(fname, "\0") || !strcmp(fname, "\n")) {
        return NULL;
    }

    //create back_store_t object in heap memory
    back_store_t *bs = (back_store_t*)malloc(sizeof(back_store_t));

    //open the file and store the file descriptor
    int flags = O_RDWR;
    int fd = open(fname, flags);
    if(fd == -1) {
        free(bs);
        return NULL;
    }
    bs->fd = fd;

    //get bitmap_data from file
    uint8_t bitmap_data[FBM_BYTES];
    read(bs->fd, bitmap_data, FBM_BYTES);

    //import the bitmap data into a new bitmap_t
    bs->fbm = bitmap_import(NUM_BLOCKS, bitmap_data);
    if(bs->fbm == NULL) {
        close(bs->fd);
        free(bs);
        return NULL;
    }

    return bs;
}

///
/// Closes and frees a back_store object
/// \param bs block_store to close
///
void back_store_close(back_store_t *const bs) {

    if(!bs) {
        return;
    }

    //set offset to beginning of file to write out FBM data
    lseek(bs->fd, 0, SEEK_SET);

    //get pointer to internal FBM data
    const uint8_t *bitmap_data = bitmap_export(bs->fbm);

    //write out fbm data to file
    write(bs->fd, bitmap_data, FBM_BYTES);

    //close the file, destroy the bitmap, and free heap memory
    close(bs->fd);
    bitmap_destroy(bs->fbm);
    free(bs);
}

///
/// Allocates a block of storage in the back_store
/// \param bs the back_store to allocate from
/// \return id of the allocated block, 0 on error
///
unsigned back_store_allocate(back_store_t *const bs) {

    if(!bs) {
        return 0;
    }

    //find the first free block
    size_t block_id = 0;
    block_id = bitmap_ffz(bs->fbm);
    if(block_id == SIZE_MAX) {
        return 0; //no free blocks
    }

    //set the bit in the fbm
    bitmap_set(bs->fbm, block_id);

    return block_id;
}

///
/// Requests the allocation of a specified block id
/// \param bs back_store to allocate from
/// \param block_id block to attempt to allocate
/// \return bool indicating allocation success
///
bool back_store_request(back_store_t *const bs, const unsigned block_id) {

    if(!bs || bitmap_test(bs->fbm, block_id)) {
        return false;
    }

    //set the bit in the fbm
    bitmap_set(bs->fbm, block_id);

    return true;
}

///
/// Releases the specified block id so it may be used later
/// \param bs back_store object
/// \param block_id block to release
///
void back_store_release(back_store_t *const bs, const unsigned block_id) {

    if(!bs || block_id < FBM_BLOCKS) {
        return;
    }

    //free the block in the fbm
    bitmap_reset(bs->fbm, block_id);
}

///
/// Reads data from the specified block to the given data buffer
/// \param bs the object to read from
/// \param block_id the block to read from
/// \param dst the buffer to write to
/// \return bool indicating success
///
bool back_store_read(back_store_t *const bs, const unsigned block_id, void *const dst) {

    if(!bs || !dst || block_id < FBM_BLOCKS || !bitmap_test(bs->fbm, block_id)) {
        return false;
    }

    //set file offset to correct block position
    lseek(bs->fd, block_id*BLOCK_SIZE, SEEK_SET);

    //read the block into dst
    read(bs->fd, (uint8_t*)dst, BLOCK_SIZE);

    return true;
}

///
/// Writes data from the given buffer to the specified block
/// \param bs the object to write to
/// \param block_id the block to write to
/// \param src the buffer to read from
/// \return bool indicating success
///
bool back_store_write(back_store_t *const bs, const unsigned block_id, const void *const src) {

    if(!bs || !src || block_id < FBM_BLOCKS || !bitmap_test(bs->fbm, block_id)) {
        return false;
    }

    //set file offset to correct block position
    lseek(bs->fd, block_id*BLOCK_SIZE, SEEK_SET);

    //write src out to block
    write(bs->fd, (const uint8_t*)src, BLOCK_SIZE);
    
    return true;
}
