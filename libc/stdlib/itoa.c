#include <string.h>

char* itoa(int value) {
	static char buffer[12];        // 12 bytes is big enough for an INT32
	int original = value;        // save original value
	
	int c = sizeof(buffer)-1;
	
	buffer[c] = 0;                // write trailing null in last byte of buffer    
	
	if (value < 0)                 // if it's negative, note that and take the absolute value
	    value = -value;
	  
	do {	// write least significant digit of value that's left
	    buffer[--c] = (value % 10) + '0';    
	    value /= 10;
	} while (value);
	
	if (original < 0) 
	    buffer[--c] = '-';
	
	return &buffer[c];
}

