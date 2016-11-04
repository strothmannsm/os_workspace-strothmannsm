#include "../include/bitmap.h"

// data is an array of uint8_t and needs to be allocated in bitmap_create
//      and used in the remaining bitmap functions. You will use data for any bit operations and bit logic   
// bit_count the number of requested bits, set in bitmap_create from n_bits
// byte_count the total number of bytes the data contains, set in bitmap_create 
struct bitmap {
	uint8_t *data;
	size_t bit_count, byte_count;
};

bitmap_t *bitmap_create(size_t n_bits) {
	
    if(n_bits == 0 || n_bits == -1) {
		return NULL;
	}
	/* allocate bitmap struct in heap memory to be returned
	*  num_ints calculation to determine the number of array items
	*  needed for n_bits
	*  calloc num_ints size array (calloc clears the memory)
	*/
	bitmap_t* bm = (bitmap_t*)malloc(sizeof(bitmap_t));
	int num_ints = ceil(n_bits / sizeof(uint8_t));
	uint8_t* bit_int_array = (uint8_t*)calloc(num_ints,sizeof(uint8_t));
    // set struct members and return the pointer
	bm->data = bit_int_array;
    bm->bit_count = n_bits;
	bm->byte_count = ceil(n_bits / 8.0);

	return bm;
}

bool bitmap_set(bitmap_t *const bitmap, const size_t bit) {
	if(!bitmap || bit == -1 || bit > bitmap->bit_count) {
		return false;
    }
    // calculate the array index and bit position in that index for requested bit
	int i = bit / (sizeof(uint8_t)*8);
	int pos = bit % (sizeof(uint8_t)*8);
    // set bit and shift to pos
	uint8_t flag = 1;
	flag = flag << pos;
	//union (or) to set bit in array
	bitmap->data[i] = bitmap->data[i] | flag;
        
	return true;
}

bool bitmap_reset(bitmap_t *const bitmap, const size_t bit) {

	if(!bitmap || bit == -1) {
		return false;
	}
    // calculate index and bit position for requested bit
	int i = bit / (sizeof(uint8_t)*8);
	int pos = bit % (sizeof(uint8_t)*8);
	// set bit, shift to pos and inverse
	uint8_t flag = 1;
	flag = flag << pos;
	flag = ~flag;
	// bit must be already set and set in flag to stay set
	// requested bit to reset is not 0 in flag, so it will be reset
	bitmap->data[i] = bitmap->data[i] & flag;
    
	return true;
}

bool bitmap_test(const bitmap_t *const bitmap, const size_t bit) {

	if(!bitmap || bit == -1) {
		return false;
	}
	//calculate index and bit position for bit
	int i = bit / (sizeof(uint8_t)*8);
	int pos = bit % (sizeof(uint8_t)*8);
	//set bit in test value and shift to pos
	uint8_t flag = 1;
	flag = flag << pos;
	//if bit is set in data[i] return true
	if(bitmap->data[i] & flag) {
		return true;
	}
    //bit not set in data[i] return false
	return false;
}

size_t bitmap_ffs(const bitmap_t *const bitmap) {
    if(!bitmap) {
        return SIZE_MAX;
    }
    /* iterate over each bit
    *  test each bit until first set bit is found by bitmap_test
    *  if/when found return bit
    */
    for(size_t bit = 0; bit < bitmap->bit_count; bit++){
        if(bitmap_test(bitmap,bit)) {
            return bit;
        }
    }
    // no set bit found, return SIZE_MAX
	return SIZE_MAX;
}

size_t bitmap_ffz(const bitmap_t *const bitmap) {
    if(!bitmap) {
        return SIZE_MAX;
    }
    /* iterate over each bit
    *  test each bit until first zero bit is found but !bitmap_test
    *	if/when found return bit
    */
    for(size_t bit = 0; bit < bitmap->bit_count; bit++){
        if(!bitmap_test(bitmap,bit)) {
            return bit;
        }
    }
    // no zero bit found, return SIZE_MAX
	return SIZE_MAX;
}

bool bitmap_destroy(bitmap_t *bitmap) {
    //make sure theres a bitmap to destroy
    if(!bitmap) {
        return false;
    }
    //free allocated memory and return
    free(bitmap->data);
    free(bitmap);
    
	return true;
}