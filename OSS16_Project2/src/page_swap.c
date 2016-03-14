#include <stdbool.h>
#include <string.h>
#include <stdlib.h>
#include <stdio.h>
#include <assert.h>
#include <stdint.h>
// link back store
#include <back_store.h>

#include "../include/page_swap.h"

// MACROS
#define MAX_PAGE_TABLE_ENTRIES_SIZE 2048
#define MAX_PHYSICAL_MEMORY_SIZE 512
#define TIME_INTERVAL 100
#define DATA_BLOCK_SIZE 1024

// helper macro
#define BS_PAGE_MAP(x) ((x) + 8);

/*
 * An individual frame
 * */
typedef struct {
	unsigned int page_table_idx; // used for indexing the page table
	unsigned char data[DATA_BLOCK_SIZE]; // the data that a frame can hold
	unsigned char access_tracking_byte; // used in LRU approx
	unsigned char access_bit; // used in LRU approx
}frame_t;

/*
 * Manages the array of frames
 * */
typedef struct {
	frame_t entries[MAX_PHYSICAL_MEMORY_SIZE]; // creates an frame array
}frame_table_t;

/*
 * An individual page
 * */
typedef struct {
	unsigned int frame_table_idx; // used for indexing the frame table
	unsigned char valid; // used to tell if the page is valid or not
} page_t;


/*
 * Manages the array of pages 
 * */
typedef struct {
	page_t entries[MAX_PAGE_TABLE_ENTRIES_SIZE]; // creates an page array
}page_table_t;


/*
 * CONTAINS ALL structures in one structure
 * */
typedef struct {

frame_table_t frame_table;
page_table_t page_table;
back_store_t* bs;

}page_swap_t;


// A Global variable that is used in the following page
// swap algorithms
static page_swap_t ps; 


// function to populate and fill your frame table and page tables
// do not remove
bool initialize (void) {

	// needs work to create back_store properly
	back_store_t* bs = back_store_create("PAGE_SWAP");
	ps.bs = bs;
	
	unsigned char buffer[1024] = {0};
	// requests the blocks needed
	for (int i = 0; i < MAX_PAGE_TABLE_ENTRIES_SIZE; ++i) {
		if(!back_store_request(ps.bs,i+8)) {
			fputs("FAILED TO REQUEST BLOCK",stderr);
			return false;
		}
		// create dummy data for blocks
		for (int j = 0; j < 1024; ++j) {
			buffer[j] = j % 255;
		}
		// fill the back store
		if (!write_to_back_store (buffer,i)) {
			fputs("FAILED TO WRITE TO BACK STORE",stderr);
			return false;
		}
	}

	/*zero out my tables*/
	memset(&ps.frame_table,0,sizeof(frame_table_t));
	memset(&ps.page_table,0,sizeof(page_table_t));

	/* Fill the Page Table and Frame Table from 0 to 512*/
	frame_t* frame = &ps.frame_table.entries[0];
	page_t* page = &ps.page_table.entries[0];
	for (int i = 0;i < MAX_PHYSICAL_MEMORY_SIZE; ++i, ++frame, ++page) {
		// update frame table with page table index
		frame->page_table_idx = i;
		// set the most significant bit on accessBit
		frame->access_bit = 128;
		// assign tracking byte to max time
		frame->access_tracking_byte = 255;
		/*
		 * Load data from back store
		 * */
		unsigned char* data = &frame->data[0];
		if (!read_from_back_store (data,i)) {
			fputs("FAILED TO READ FROM BACK STORE",stderr);
			return false;
		}
		// update page table with frame table index
		page->frame_table_idx = i;
		page->valid = 1;
		
	}
	return true;
}
// keep this do not delete
void destroy(void) {
	back_store_close(ps.bs);
}

/*
 * ALRU IMPLEMENTATION : TODO IMPLEMENT
 * */

page_request_result_t* approx_least_recently_used (const uint16_t page_number, const size_t clock_time) {		
	page_request_result_t* page_req_result = NULL;
	return page_req_result;
}


/*
 * LFU IMPLEMENTATION : TODO IMPLEMENT
 * */
page_request_result_t* least_frequently_used (const uint16_t page_number, const size_t clock_time) {
	page_request_result_t* page_req_result = NULL;
	return page_req_result;
}


/*
 * BACK STORE WRAPPER FUNCTIONS: TODO IMPLEMENT
 * */
bool read_from_back_store (void *data, const unsigned int page) {
	return false;
}

bool write_to_back_store (const void *data, const unsigned int page) {
	return false;
}
