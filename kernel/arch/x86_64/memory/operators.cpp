#include<stddef.h>
#include<memory.h>
#include<new>
#include<kernel/safe_ptr.hpp>

void* operator new(size_t size) {
	return Memory::Paging::kalloc(size, 1);
}

void* operator new[](size_t size) {
	return Memory::Paging::kalloc(size, 1);
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
