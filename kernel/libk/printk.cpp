#include <cstdint>
#include <cstddef>
#include <string.h>
#include <kernel.h>
#include <stdlib.h>
#include <stdio.h>
#include <cstdarg>
#include <kernel/streams.h>
#include <kernel/interrupts.h>
#include <kernel/drivers/framebuffer.hpp>

void logk(char* msg, enum LOGLEVEL level) {
	if (msg != nullptr) {
		switch(level) {
			case LOGLEVEL::KERNEL:
				printfk("[KERNEL] %s", msg);
				break;
			case LOGLEVEL::USER:
				printfk("[USER] %s", msg);
				break;
			case LOGLEVEL::INFO:
				printfk("[INFO] %s", msg);
				break;
			case LOGLEVEL::ERROR:
				printfk("[ERROR] %s", msg);
				break;
			case LOGLEVEL::PANIC:
				printfk("[PANIC] %s", msg);
				break;
			default:
				printk(msg);
		}
	} else {
		printk("Error in logk: msg is null!");
	}

}


void printfk(char* format...) {
	va_list args;
	va_start(args, format);

	char* cur = format;
	uint64_t idx = 0;
	while(*cur != '\0') {
		if (*cur == '%') {
			cur++;
			if (*cur == 'd') {
				int64_t arg = va_arg(args, int64_t);
				printk(itoa(arg));
			} else if (*cur == 'u') {
				uint64_t arg = va_arg(args, uint64_t);
				printk(uitoa(arg));
			} else if (*cur == 'x') {
				uint64_t arg = va_arg(args, uint64_t);
				printk(hex_to_str(arg));
			} else if (*cur == 's') {
				char* arg = va_arg(args, char*);
				printk(arg);
			} else if (*cur == 'c') {
				int arg = va_arg(args, int);
				putchk(arg);
				if (arg == '\b') {
					putchk(' ');
					putchk(arg);
				}
				
			} else if (*cur == '%') {
				putchk("%");
			} else {
				putchk("%");
				putchk(*cur);
			}
		} else {
			putchk(*cur);
		}
		cur++;
	}
	va_end(args);
}

void putchk(char c) {
	FrameBuffer::write(&c, 1);
}

void printk(char* msg) {
	if (msg != nullptr) {
		FrameBuffer::write(msg, strlen(msg));
	} else {
		char* m = "Error in printk: msg is null!";
		FrameBuffer::write(m, strlen(m));
	}
	return;
}

uint8_t getch() {
	//wait for stdin to be populated
	while (Streams::stdin().length() < 1) {}
	uint8_t c = Streams::stdin().pop_head();
	return c;
}


