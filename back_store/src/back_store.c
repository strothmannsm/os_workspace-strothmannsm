#include "../include/back_store.h"

#include <bitmap.h>

#include <fcntl.h>
#include <stdbool.h>
#include <sys/mman.h>
#include <sys/stat.h>
#include <sys/types.h>
#include <unistd.h>

#define BLOCK_COUNT 65536
#define BLOCK_SIZE 1024
#define FBM_BLOCK_COUNT 8
#define BYTE_TOTAL (65536 * 1024)
#define DATA_BLOCK_BYTE_TOTAL ((65536 - 8) * 1024)
#define FBM_BYTE_TOTAL (1024 * 8)
#define DATA_BLOCK_START 8


struct back_store {
    int fd;
    bitmap_t *fbm;
    uint8_t *data_blocks;
};

int create_file(const char *const fname) {
    if (fname) {
        int fd = open(fname, O_RDWR | O_CREAT | O_TRUNC, S_IRUSR | S_IWUSR | S_IRGRP | S_IROTH);
        if (fd != -1) {
            if (ftruncate(fd, BYTE_TOTAL) != -1) {
                return fd;
            }
            close(fd);
        }
    }
    return -1;
}
int check_file(const char *const fname) {
    if (fname) {
        int fd = open(fname, O_RDWR, S_IRUSR | S_IWUSR | S_IRGRP | S_IROTH);
        if (fd != -1) {
            struct stat file_info;
            if (fstat(fd, &file_info) != -1 && file_info.st_size == BYTE_TOTAL) {
                return fd;
            }
            close(fd);
        }
    }
    return -1;
}


back_store_t *back_store_init(const bool init, const char *const fname) {
    if (fname) {
        back_store_t *bs = (back_store_t *) malloc(sizeof(back_store_t));
        if (bs) {
            bs->fd = init ? create_file(fname) : check_file(fname);
            if (bs->fd != -1) {
                bs->data_blocks = (uint8_t *) mmap(NULL, BYTE_TOTAL, PROT_READ | PROT_WRITE, MAP_SHARED, bs->fd, 0);
                if (bs->data_blocks != (uint8_t *) MAP_FAILED) {
                    // Woo hoo! Done. Mostly. Kinda.
                    if (init) {
                        // init FBM and wipe remaining data
                        // Could/should be done in create_file
                        // but it's so much easier here...
                        memset(bs->data_blocks, 0xFF, FBM_BLOCK_COUNT >> 3);
                        memset(bs->data_blocks + FBM_BYTE_TOTAL, 0x00, DATA_BLOCK_BYTE_TOTAL);
                    }
                    // Not quite sure what to do with madvise
                    // Honestly, I feel like a split mapping may be best
                    // Sequential for the FBM, random for the data
                    // but I'll just not mess with it unless I get the time to profile them
                    // madvise()
                    bs->fbm = bitmap_overlay(BLOCK_COUNT, bs->data_blocks);
                    if (bs->fbm) {
                        return bs;
                    }
                    munmap(bs->data_blocks, BYTE_TOTAL);
                }
                close(bs->fd);
            }
            free(bs);
        }
    }
    return NULL;
}

back_store_t *back_store_create(const char *const fname) {
    return back_store_init(true, fname);
}

back_store_t *back_store_open(const char *const fname) {
    return back_store_init(false, fname);
}

void back_store_close(back_store_t *const bs) {
    if (bs) {
        bitmap_destroy(bs->fbm);
        munmap(bs->data_blocks, BYTE_TOTAL);
        close(bs->fd);
        free(bs);
    }
}

unsigned back_store_allocate(back_store_t *const bs) {
    if (bs) {
        size_t free_block = bitmap_ffz(bs->fbm);
        if (free_block != SIZE_MAX) {
            bitmap_set(bs->fbm, free_block);
            return free_block;
        }
    }
    return 0;
}

bool back_store_request(back_store_t *const bs, const unsigned block_id) {
    if (bs && block_id >= DATA_BLOCK_START && block_id <= BLOCK_COUNT) {
        if (!bitmap_test(bs->fbm, block_id)) {
            bitmap_set(bs->fbm, block_id);
            return true;
        }
    }
    return false;
}

void back_store_release(back_store_t *const bs, const unsigned block_id) {
    if (bs && block_id >= DATA_BLOCK_START && block_id <= BLOCK_COUNT) {
        bitmap_reset(bs->fbm, block_id);
    }
}

bool back_store_read(back_store_t *const bs, const unsigned block_id, void *const dst) {
    if (bs && dst && block_id >= DATA_BLOCK_START && block_id <= BLOCK_COUNT /* && bitmap_set(bs->fbm,block_id) */) {
        memcpy(dst, bs->data_blocks + (BLOCK_SIZE * block_id), BLOCK_SIZE);
        return true;
    }
    return false;
}


bool back_store_write(back_store_t *const bs, const unsigned block_id, const void *const src) {
    if (bs && src && block_id >= DATA_BLOCK_START && block_id <= BLOCK_COUNT /* && bitmap_set(bs->fbm,block_id) */) {
        memcpy(bs->data_blocks + (BLOCK_SIZE * block_id), src, BLOCK_SIZE);
        return true;
    }
    return false;
}
