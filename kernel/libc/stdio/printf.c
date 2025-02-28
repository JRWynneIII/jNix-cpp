#include<stdlib.h>
#include<stdio.h>

//TODO:
//	malloc
//	sprintf
//	printf? or kprintf

char* hex_to_str(unsigned long input) {
	char* hex = "0123456789ABCDEF";
	static char temp[17];
	static char ret[17];

	if (input <= 0) {
		for (int i = 0; i < 17; i++) {
			temp[i] = '0';
		}
		temp[16] = 0;
		return temp;
	}

	int idx = 0;
	while (input > 0) {
		temp[idx] = hex[input & 0xF];
		idx++;
		input = input >> 4;
	}

	for (int i = 0; i<idx; i++) {
		ret[i] = temp[idx - i - 1];
	}

	ret[idx] = 0;
	int len = strlen(ret);
	if (len < 16) {
		int num_zeros = 16-len;
		char t[len+1];
		for (int i = 0; i < len; i++) {
			t[i] = ret[i];
		}
		t[len] = 0;
		for (int i = 0; i < num_zeros; i++) {
			ret[i] = '0';
		}

		int index = 0;
		for (int i = num_zeros; i < 17; i++) {
			ret[i] = t[index];
			index++;
		}
	}

	return ret;
}

