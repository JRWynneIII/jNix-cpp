#pragma once
#include<stdint.h>
#include<vector.hpp>
#include<string.hpp>
//#include<kernel/interrupts.h>
//#include<kernel/drivers/driver.hpp>
//#include<kernel/drivers/framebuffer.hpp>

void printk(char* msg);
void putchk(char c);
void printfk(char* format...);
char* sprintf(char* format...);
uint8_t getch();

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

void halt();

namespace Kernel {
	vector<String>& kernel_logs();
	void append_log(String s); 

	template<typename... varargs>
	void Log(enum LOGLEVEL level, char* fmt, varargs... args) {
		if (fmt != nullptr) {
			String fullmsg;
			switch(level) {
				case LOGLEVEL::KERNEL:
					fullmsg = "[KERNEL] ";
					//TODO: fix sprintf so that you pass a buffer or a size argument
					//	this is causing corruption of the slab table somehow??
					fullmsg += sprintf(fmt, args...);
					break;
				case LOGLEVEL::USER:
					fullmsg = "[USER] ";
					fullmsg += sprintf(fmt, args...);
					break;
				case LOGLEVEL::INFO:
					fullmsg = "[INFO] ";
					fullmsg += sprintf(fmt, args...);
					break;
				case LOGLEVEL::ERROR:
					fullmsg = "[ERROR] ";
					fullmsg += sprintf(fmt, args...);
					break;
				case LOGLEVEL::PANIC:
					fullmsg = "[PANIC] ";
					fullmsg += sprintf(fmt, args...);
					break;
				default:
					printk(fmt);
			}
			Kernel::append_log(fullmsg);
			printfk("%s", fullmsg.cstring());
		} else {
			printk("Error in logk: fmt is null!");
		}
	
	}
}
	
