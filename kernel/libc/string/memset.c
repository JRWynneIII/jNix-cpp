#include <stdbool.h> /* C doesn't have booleans by default. */
#include <stddef.h>

//void *memset(void *dest, char val, size_t count)
//{
//    char *temp = (char *)dest;
//    for( ; count != 0; count--) *temp++ = val;
//    return dest;
//}

void* memset(void* memory, int value, size_t length) {
    char* p = (char*)memory;
    char c = (char)value;

    while (length-- > 0) *p++ = c;

    return memory;
}

unsigned short *memsetw(unsigned short *dest, unsigned short val, size_t count)
{
    unsigned short *temp = (unsigned short *)dest;
    for( ; count != 0; count--) *temp++ = val;
    return dest;
}
