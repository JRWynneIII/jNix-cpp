#include<cstdint>
#include<string.h>

int atoi(char* input) {
	int ret = 0;
	// Skip whitespaces;
	while(input[0] == ' ' && input[0] != '\0') input++;

	//Early return if its null terminated
	if (input[0] == '\0') return 0;

	//Check if value is negative or explicitly positive
	bool isnegative = false;
	if (input[0] == '-') {
		isnegative = true;
		input++;
	} else if (input[0] == '+') {
		//Ignore the +
		input++;
	}

	//Parse
	while (input[0] != '\0') {
		//TODO: Check for overflow?
		ret = ret * 10 + (input[0] - '0');
		input++;
	}

	return ret;
}
