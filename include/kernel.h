#pragma once
#include<stdint.h>

void printk(char* msg);

enum LOGLEVEL {
	KERNEL,
	USER,
	INFO,
	ERROR,
	PANIC,
	NONE
};

void logk(char* msg, enum LOGLEVEL level);

namespace FrameBuffer {
	void init();
}

void halt();
