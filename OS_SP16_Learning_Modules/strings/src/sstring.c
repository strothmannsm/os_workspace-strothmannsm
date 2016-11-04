#include <string.h>
#include <errno.h>
#include <stdio.h>
#include <limits.h>


#include "../include/sstring.h"

bool string_valid(const char *str, const size_t length) {
	//check for invalid inputs
	if(!str || length == 0) {
		return false;
	}
	//check for null terminator, return true if found
	if(str[length-1] == '\0') {
		return true;
	}
	//null terminator not found, invalid string, return false
	return false;
}

char *string_duplicate(const char *str, const size_t length) {
	//check for invalid inputs
	if(!string_valid(str,length)) {
		return NULL;
	}
	//create and malloc new char* to copy string to
	char *str_copy = (char*)malloc(length * sizeof(char));
	//verify malloc
	if(str_copy == NULL) {
		return NULL;
	}
	//copy bytes
	memcpy(str_copy, str, length);

	return str_copy;

}

bool string_equal(const char *str_a, const char *str_b, const size_t length) {	
	//check for invalid inputs
	if(!string_valid(str_a,length) || !string_valid(str_b,length)) {
		return false;
	}
	//strcmp returns 0 for equal strings, return true
	if(!strcmp(str_a, str_b)) {
		return true;
	}
	//strings not equal, return false
	return false;
}

int string_length(const char *str, const size_t length) {
	//check for invalid inputs
	if(!str || length == 0) {
		return -1;
	}
	//return length
	return strlen(str); 
}

int string_tokenize(const char *str, const char *delims, const size_t str_length,char **tokens, const size_t max_token_length, const size_t requested_tokens) {
	//check for invalid inputs
	if(!string_valid(str,str_length) || !delims || !tokens || max_token_length == 0 || requested_tokens == 0) {
		return 0;
	}	
	// create and malloc char* and copy str to be able to use strtok since 
	// strtok can't use constant str pointer
	// free before return if successful or tokens member un-allocated
	char *str_copy = (char*)malloc(str_length * sizeof(char));
	strcpy(str_copy, str);

	int i = 0;
	char *token;
	//get first token
	token = strtok(str_copy, delims);
	//loop through until end, copy current token to tokens array and count using i, get next token
	while(token != NULL) {
		if(tokens[i] == NULL) {
			free(str_copy);
			return -1;
		}
		strcpy(tokens[i], token);
		i++;
		token = strtok(NULL, delims);
	}
	//free copied str
	free(str_copy);
	//i is the count of tokens, return
	return i;
}

bool string_to_int(const char *str, int *converted_value) {
	//check for invalid inputs
	if(!str || !converted_value) {
		return false;
	}
	char *end;
	errno = 0;
	int n = strtol(str,&end,0);
	// no conversion performed if end==str
	// strtol sets errno to ERANGE if str contains "value" outside range
	if(end == str || errno == ERANGE) {
		return false;
	}
	// everything worked, set converted value and return true
	*converted_value = n;
	return true;
}