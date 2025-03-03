#include<stddef.h>
#include<memory.h>
#include<new>

void* operator new(size_t size) {
	return kmalloc(size);
}

void* operator new[](size_t size) {
	return kmalloc(size);
}

void operator delete(void* ptr) {
	kfree(ptr);
}

void operator delete[](void* ptr) {
	kfree(ptr);
}

void operator delete(void* ptr, size_t s) {
	kfree(ptr);
}

void operator delete[](void* ptr, size_t s) {
	kfree(ptr);
}
