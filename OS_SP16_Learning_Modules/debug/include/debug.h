#ifndef OS_DEBUG_H_
#define OS_DEBUG_H_

#include <stdbool.h>
// NOTE USE VALGRIND AND GDB to help find memory leaks and code errors


// sorts an array and validates the array upto value_count
// \param data_array the array that will be sorted using quicksort
// \param value_count the number of elements in the data_array
// \return true for successful sort, false for errors and unsuccessful 
bool terrible_sort(uint16_t *data_array, const size_t value_count);

#endif
