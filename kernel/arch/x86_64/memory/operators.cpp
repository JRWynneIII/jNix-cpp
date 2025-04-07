#include<stddef.h>
#include<kernel/memory.h>
#include<new>
#include<kernel/ptr.hpp>

void* operator new(size_t size) {
	return Memory::Allocation::kallocate(size, 1);
}

void* operator new(unsigned long size, std::align_val_t a) {
	return Memory::Allocation::kallocate(size, 1, true);
}

void* operator new[](size_t size) {
	return Memory::Allocation::kallocate(size, 1);
}

void operator delete(void* ptr) {
	Memory::Allocation::kfree(ptr);
}

void operator delete[](void* ptr) {
	Memory::Allocation::kfree(ptr);
}

void operator delete(void* ptr, size_t s) {
	Memory::Allocation::kfree(ptr);
}

void operator delete[](void* ptr, size_t s) {
	Memory::Allocation::kfree(ptr);
}

void operator delete(void* ptr, unsigned long s, std::align_val_t a) {
	Memory::Allocation::kfree(ptr);
}
