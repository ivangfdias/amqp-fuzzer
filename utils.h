#include <string.h>
#include <stddef.h>
#include <stdlib.h>



// Converts src[index], src[index+1] to a short
short char_in_short( unsigned char* src, int index);

// Converts src[index]..src[index+3] to an int
int char_in_int(unsigned char* src, int index);

// Converts src[index]..src[index+7] to a long
long char_in_long(unsigned char* src, int index);

// Does the opposite
void long_in_char(unsigned char * dst, const long src, int index, size_t n);

void int_in_char(unsigned char * dst, const int src, int index, size_t n);

void short_in_char(unsigned char * dst, const short src, int index, size_t n);

unsigned char* extract_char_array_entry(unsigned char* src, int index);

unsigned char* extract_string_entry(unsigned char* src, int index);

char check_bit_equal(unsigned short arg1, unsigned short arg2);