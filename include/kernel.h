#pragma once
#include<stdint.h>

void printk(char* msg);
void putchk(char c);
void printfk(char* format...);

enum LOGLEVEL {
	KERNEL,
	USER,
	INFO,
	ERROR,
	PANIC,
	NONE
};

void logk(char* msg, enum LOGLEVEL level);

// TODO: Don't define this here this is garbage. But since templated functions have to be defined in a header file
// 	we are limited here on being able to define this in a c++ file. I guess i just don't know C++ as well as
// 	i thought i did
template<typename... varargs>
void logfk(enum LOGLEVEL level, char* fmt, varargs... args) {
	if (fmt != nullptr) {
		switch(level) {
			case LOGLEVEL::KERNEL:
				printk("[KERNEL] ");
				printfk(fmt, args...);
				break;
			case LOGLEVEL::USER:
				printk("[USER] ");
				printfk(fmt, args...);
				break;
			case LOGLEVEL::INFO:
				printk("[INFO] ");
				printfk(fmt, args...);
				break;
			case LOGLEVEL::ERROR:
				printk("[ERROR] ");
				printfk(fmt, args...);
				break;
			case LOGLEVEL::PANIC:
				printk("[PANIC] ");
				printfk(fmt, args...);
				break;
			default:
				printk(fmt);
		}
	} else {
		printk("Error in logk: fmt is null!");
	}

}

namespace FrameBuffer {
	void init();
}

void halt();
