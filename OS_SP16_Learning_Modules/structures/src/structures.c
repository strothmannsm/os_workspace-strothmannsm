
#include "../include/structures.h"

int compare_structs(sample_t* a, sample_t* b)
{
    if(!a || !b) {
        return 0;
    }
    // compare each member of a to b, if all equal, structs equal, return 1
    if(a->a == b->a && a->b == b->b && a->c == b->c) {
        return 1;
    }
    // something not equal return 0;
	return 0;
}

void print_alignments()
{
	printf("Alignment of int is %zu bytes\n",__alignof__(int));
	printf("Alignment of double is %zu bytes\n",__alignof__(double));
	printf("Alignment of float is %zu bytes\n",__alignof__(float));
	printf("Alignment of char is %zu bytes\n",__alignof__(char));
	printf("Alignment of long long is %zu bytes\n",__alignof__(long long));
	printf("Alignment of short is %zu bytes\n",__alignof__(short));
	printf("Alignment of structs are %zu bytes\n",__alignof__(fruit_t));
}

int sort_fruit(const fruit_t* a,int* apples,int* oranges, const size_t size)
{
	if(!a || !apples || !oranges || size == 0) {
		return -1;
	}
	//check type of every fruit and increment respective counter
	for(int i = 0; i < size; i++) {
		if(a[i].type == APPLE) {
			(*apples)++;
		} else {
			(*oranges)++;
		}
	}
	// return sum of counters (total number of items)
	return *apples + *oranges;
}

int initialize_array(fruit_t* a, int apples, int oranges)
{
	if(!a || (apples == 0 && oranges == 0)) {
		return -1;
	}
	// create requested number of apples and put into a
	for(int i = 0; i < apples; i++) {
		fruit_t apple;
		int test = initialize_apple((apple_t*)&apple);
		if(test == -1) {
			return test;
		}
		a[i] = apple;
	}
	// create requested number of oranges and put into a
	for(int i = 0; i < oranges; i++) {
		fruit_t orange;
		int test = initialize_orange((orange_t*)&orange);
		if(test == -1) {
			return test;
		}
		a[apples + i] = orange;
	}

	return 0;
}

int initialize_orange(orange_t* a)
{	
	if(!a) {
		return -1;
	}
	//set struct members in a and return 0 for success
	a->type = ORANGE;
	a->weight = 0;
	a->peeled = 0;

	return 0;
}

int initialize_apple(apple_t* a)
{	
	if(!a) {
		return -1;
	}
	//set struct members in a and return 0 for success
	a->type = APPLE;
	a->weight = 0;
	a->worms = 0;

	return 0;
}
