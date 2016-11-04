#include <string.h>
#include <stdlib.h>
#include <stdio.h>
#include <errno.h>
#include <unistd.h>
#include <sys/stat.h>
#include <fcntl.h>

#include "../include/sys_prog.h"

// LOOK INTO OPEN, READ, WRITE, CLOSE, FSTAT/STAT, LSEEK
// GOOGLE FOR ENDIANESS HELP

bool bulk_read(const char *input_filename, void *dst, const size_t offset, const size_t dst_size) {

	if(!input_filename || !dst || dst_size == 0) {
		return false;
	}
	// open file in read mode
	int fd = open(input_filename, O_RDONLY);
	// open returns -1 on problem
	if(fd == -1) {
		return false;
	}
	// lseek to offset
	lseek(fd, offset, SEEK_CUR);
	//read the file into dst for dst_size bytes, -1 return on error, return false
	if(read(fd, dst, dst_size) == -1) {
		close(fd);
		return false;
	}
	// read successful, close file and return true
	close(fd);
	return true;
}

bool bulk_write(const void *src, const char *output_filename, const size_t offset, const size_t src_size) {

	if(!output_filename || !src || src_size == 0 || !strcmp(output_filename,"") || !strcmp(output_filename,"\n")) {
		return false;
	}
	//open file in write mode
	int fd = open(output_filename, O_CREAT | O_TRUNC | O_WRONLY);

	if(fd == -1) {
		return false;
	}
	// lseek to offset
	lseek(fd, offset, SEEK_CUR);
	// write from src to file for src_size bytes, -1 returned on error
	if(write(fd, src, src_size) == -1) {
		close(fd);
		return false;
	}
	// write successful, close file and return true
	close(fd);
	return true;
}


bool file_stat(const char *query_filename, struct stat *metadata) {

	if(!query_filename || !metadata) {
		return false;
	}
	// stat returns 0 on success, return true
	if(stat(query_filename, metadata) == 0) {
		return true;
	}
	// stat unsuccesful, return false
	return false;
}

bool endianess_converter(uint32_t *src_data, uint32_t *dst_data, const size_t src_count) {

    if(!src_data || !dst_data || src_count == 0) {
        return false;
    }
    // for each element in src_data, save to data (temporary), swap endianness, and put into dst_data
    for(int i = 0; i < src_count; i++) {
    	uint32_t data = src_data[i];
    	data = ((data << 8) & 0xFF00FF0) | ((data >> 8) & 0xFF00FF);
    	dst_data[i] = (data << 16) | ((data >> 16) & 0xFFFF);
    }
    
	return true;
}
