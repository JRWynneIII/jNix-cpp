#ifndef JNIX_H
#define JNIX_H
void init_idt();
void printk(char* msg);

enum LOGLEVEL {
	KERNEL,
	USER,
	INFO,
	ERROR
};

void logk(char* msg, enum LOGLEVEL level);
void init_framebuf();
void init_term();
#endif
