#include <stddef.h>
#include <stdbool.h>

bool strcmp(char* lhs, char* rhs) {
	size_t idx = 0;
	while(lhs[idx] != 0 || rhs[idx] != 0) {
		if (lhs[idx] > rhs[idx]) return false;
		if (lhs[idx] < rhs[idx]) return false;
		if ((lhs[idx] == 0 && rhs[idx] != 0) || (rhs[idx] == 0 && lhs[idx] != 0)) return false;
		idx++;
	}
	return true;
}


