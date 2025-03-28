#include<cstdint>
#include<string.h>

char* strcat(char* lhs, char* rhs) {
	uint64_t lhs_size = strlen(lhs);
	uint64_t rhs_size = strlen(rhs);
	uint64_t new_size = lhs_size + rhs_size;
	char* buf = new char[new_size + 1];
	for(uint64_t i = 0; i < lhs_size; i++) buf[i] = lhs[i];
	for(uint64_t i = 0; i < rhs_size; i++) buf[i+lhs_size] = rhs[i];
	buf[new_size] = '\0';
	return buf;
}
