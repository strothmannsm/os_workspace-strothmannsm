#include "../include/allocation.h"

void* allocate_array(size_t member_size, size_t nmember,bool clear)
{
    if(member_size == 0 || nmember == 0 || member_size == -1 || nmember == -1) {
        return NULL;
    }
    
    void* ret;
    //if requested clear memory calloc, else malloc
    if(clear){
        ret = calloc(nmember, member_size);
    } else {
        ret = malloc(nmember * member_size);
    }
    //check for successful allocation
    if(!ret) {
        return NULL;
    }
    
	return ret;
}

void* reallocate_array(void* ptr, size_t size)
{   
    if(!ptr || size == 0 || size == -1) {
        return NULL;
    }
    //attempt to realloc and store in temp pointer
    void* temp = realloc(ptr, size);
    //creturn base on realloc results, NULL means failed, ==ptr means successful at same address
    //otherwise new address returned
    if(temp == NULL) {
        return NULL;
    } else if(temp == ptr) {
        return ptr;
    } else {
        return temp;
    }
}

void deallocate_array(void** ptr)
{
    if((*ptr) == NULL) {
        return;
    }
    // free the memory and reset to NULL
    free(*ptr);
    *ptr = NULL;
    
    return;
}

char* read_line_to_buffer(char* filename)
{
    if(!filename) {
        return NULL;
    }
    //allocate buffer space
    char *buffer = (char*)malloc(BUFSIZ * sizeof(char));
    //open the file, read a line into buffer and close the file
    FILE* fp = fopen(filename,"r");
    fread(buffer,sizeof(char),BUFSIZ,fp);
    fclose(fp);

	return buffer;
}
