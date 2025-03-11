#include<stdio.h>
#include<string.h>
#include<cstdarg>
#include<cstdint>
#include<stdlib.h>

char* sprintf(char* format...) {
	va_list args;
	va_start(args, format);
	char* ret = new char[512];
	uint64_t ret_idx = 0;

	char* cur = format;
	uint64_t idx = 0;
	while(*cur != '\0') {
		if (*cur == '%') {
			cur++;
			if (*cur == 'd') {
				int64_t arg = va_arg(args, int64_t);
				char* val = itoa(arg);
				for (int i = 0; i < strlen(val); i++) {
					ret[ret_idx] = val[i];
					ret_idx++;
				}
			} else if (*cur == 'u') {
				uint64_t arg = va_arg(args, uint64_t);
				char* val = uitoa(arg);
				for (int i = 0; i < strlen(val); i++) {
					ret[ret_idx] = val[i];
					ret_idx++;
				}
			} else if (*cur == 'x') {
				uint64_t arg = va_arg(args, uint64_t);
				char* val = hex_to_str(arg);
				for (int i = 0; i < strlen(val); i++) {
					ret[ret_idx] = val[i];
					ret_idx++;
				}
			} else if (*cur == 's') {
				char* val = va_arg(args, char*);
				for (int i = 0; i < strlen(val); i++) {
					ret[ret_idx] = val[i];
					ret_idx++;
				}
			} else if (*cur == 'c') {
				int arg = va_arg(args, int);
				ret[ret_idx] = arg;
				ret_idx++;
			} else if (*cur == '%') {
				ret[ret_idx] = '%';
				ret_idx++;
			} else {
				ret[ret_idx] = '%';
				ret_idx++;
				ret[ret_idx] = *cur;
				ret_idx++;
			}
		} else {
			ret[ret_idx] = *cur;
			ret_idx++;
		}
		cur++;
	}
	va_end(args);
	return ret;
}
