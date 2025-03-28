#include<cstdint>
#include<string.h>

int atoi(char* input) {
	int ret = 0;
	while(input[0] == ' ' && input[0] != '\0') input++;

	if (input[0] == '\0') return 0;

	bool isnegative = false;
	if (input[0] == '-') {
		isnegative = true;
		input++;
	} else if (input[0] == '+') {
		//Ignore the +
		input++;
	}

	while (input[0] != '\0') {
		//TODO: Check for overflow?
		ret *= 10 + (input[0] - '0');
		input++;
	}
	return ret;
}
