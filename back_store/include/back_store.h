#ifndef _BACK_STORE_H__
#define _BACK_STORE_H__

#include <stdbool.h>

// Back store object
// It's an opaque object whose implementation is up to you
// (and implementation DOES NOT go here)
typedef struct back_store back_store_t;

///
/// Creates a new back_store file at the specified location
///  and returns a back_store object linked to it
/// \param fname the file to create
/// \return a pointer to the new object, NULL on error
///
back_store_t *back_store_create(const char *const fname);

///
/// Opens the specified back_store file
///  and returns a back_store object linked to it
/// \param fname the file to open
/// \return a pointer to the new object, NULL on error
///
back_store_t *back_store_open(const char *const fname);

///
/// Closes and frees a back_store object
/// \param bs block_store to close
///
void back_store_close(back_store_t *const bs);

///
/// Allocates a block of storage in the back_store
/// \param bs the back_store to allocate from
/// \return id of the allocated block, 0 on error
///
unsigned back_store_allocate(back_store_t *const bs);

///
/// Requests the allocation of a specified block id
/// \param bs back_store to allocate from
/// \param block_id block to attempt to allocate
/// \return bool indicating allocation success
///
bool back_store_request(back_store_t *const bs, const unsigned block_id);

///
/// Releases the specified block id so it may be used later
/// \param bs back_store object
/// \param block_id block to release
///
void back_store_release(back_store_t *const bs, const unsigned block_id);

///
/// Reads data from the specified block to the given data buffer
/// \param bs the object to read from
/// \param block_id the block to read from
/// \param dst the buffer to write to
/// \return bool indicating success
///
bool back_store_read(back_store_t *const bs, const unsigned block_id, void *const dst);

///
/// Writes data from the given buffer to the specified block
/// \param bs the object to write to
/// \param block_id the block to write to
/// \param src the buffer to read from
/// \return bool indicating success
///
bool back_store_write(back_store_t *const bs, const unsigned block_id, const void *const src);

#endif