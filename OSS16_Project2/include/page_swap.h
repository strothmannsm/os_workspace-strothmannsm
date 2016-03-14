#ifndef _PAGE_SWAP_H_
#define _PAGE_SWAP_H_

#include <stdbool.h>

/*
 * Returned by all page swap algorithms
 * */
typedef struct {
	unsigned short page_requested;
	unsigned short frame_replaced;
	unsigned short page_replaced;
}page_request_result_t;

// Updates the frame table and page table using the page swap
// algorithm Least Frequently Used. Using a accessbit that is updated
// every time a page is referenced and a tracking byte for finding the minimum frame count for
// updating.
// @param page_number a page number that could be referenced
// return The page referenced, the page replaced, and the frame updated
//        or a null pointer for no page fault
//
page_request_result_t* least_frequently_used(const uint16_t page_number, const size_t clock_time);

// Updates the frame table and page table using the page swap
// algorithm Approximately Least Recently Used. Using a accessbit that is updated
// every time a page is referenced and a tracking byte for finding the minimum frame for
// updating.
// @param pageNumber a page number that could be referenced 
// @return The page referenced, the page replaced, and the frame updated
// 		   or a null pointer for no page fault 
//
page_request_result_t* approx_least_recently_used (const uint16_t page_number, const size_t clock_time);

// Reads a 1024 block of data from the back store into a an array data given a page index 
// @param data used for storage of the copied data from the back store
// @param page a logical index that references a 1024 block of data in the back store
// @return true upon successful read of 1024 from the given page or false upon failure to read 1024
//
bool read_from_back_store (void *data, const unsigned int page);

// Writes a 1024 block of data from the array data into the back store at the given page index
// @param data contains data that will be stored into the back store
// @param page a logical index that references a 1024 block of data in the back store
// @return true upon successful writes of 1024 from the given page or false upon failure to write 1024
bool write_to_back_store (const void *data, const unsigned int page);

// Bulk loads and initializes the frame table, page table, abd back_store
// return true upon a successful load and initializations or false for failure
bool initialize (void); 

//Detatches the back_store
void destroy(void);

#endif
