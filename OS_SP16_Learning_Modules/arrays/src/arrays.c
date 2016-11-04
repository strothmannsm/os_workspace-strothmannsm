#include <string.h>
#include <stdlib.h>
#include <stdio.h>

#include "../include/arrays.h"

// LOOK INTO MEMCPY, MEMCMP, FREAD, and FWRITE

bool array_copy(const void *src, void *dst, const size_t elem_size, const size_t elem_count) {
    
    if(!src || !dst || elem_size == 0 || elem_count == 0) {
        return false;
    }
    //copy elem_size*elem_count bytes from src to dst
    memcpy(dst, src, elem_size * elem_count);

	return true;
}

bool array_is_equal(const void *data_one, void *data_two, const size_t elem_size, const size_t elem_count) {
    
    if(!data_one || !data_two || elem_size == 0 || elem_count == 0) {
        return false;
    }
    //compare data_one to data_two for elem_size*elem_count bytes
    //memcmp returns 0 on equal comparison
    if(memcmp(data_one, data_two, elem_size * elem_count) == 0) {
        return true;
    }
    
	return false;
}

ssize_t array_locate(const void *data, const void *target, const size_t elem_size, const size_t elem_count) { 
    
    if(!data || !target || elem_size == 0 || elem_count == 0) {
		return -1;
	}
	/* itereate over the length of array
	*  compare elem_size bytes to target for each element
	*  individual elements located at data + (i * elem_size)
	*  if target found i is the postion, return
	*/
	for(int i = 0; i < elem_count; i++) {
		if(memcmp(target, ((char*)data + (i * elem_size)), elem_size) == 0) {
			return i;
		}
	}	
	// target not found, return -1
	return -1;
}

bool array_serialize(const void *src_data, const char *dst_file, const size_t elem_size, const size_t elem_count) {
    
    if(!src_data || !dst_file || elem_size == 0 || elem_count == 0 || !strcmp(dst_file,"") || !strcmp(dst_file,"\n")) {
		return false;
	}
	//open file and check to make sure it opened
	FILE* fp = fopen(dst_file,"w");
	if(!fp) {
		return false;
	}
	//write bytes to binary file and close
	fwrite(src_data, elem_size, elem_count, fp);
	fclose(fp);

	return true;
}

bool array_deserialize(const char *src_file, void *dst_data, const size_t elem_size, const size_t elem_count) {
    
    if(!src_file || !dst_data || elem_size == 0 || elem_count == 0 || !strcmp(src_file,"") || !strcmp(src_file,"\n")) {
		return false;
	}
	//open file and check to make sure it opened
	FILE* fp = fopen(src_file,"r");
	if(!fp) {
		return false;
	}
	//read from file and close
	fread(dst_data, elem_size, elem_count, fp);
	fclose(fp);
	
	return true;
}
