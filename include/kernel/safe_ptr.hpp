#pragma once
#include<unwind.h>
#include<kernel.h>

template<typename T>
class safe_ptr {
private:
	T* ptr;
	uint64_t end;
	uint64_t num_elements;
public:
	safe_ptr<T>(T* p, size_t size) {
		this->ptr = p;
		this->end = size;
		this->num_elements = size / sizeof(T);
		for (int i = 0; i < (this->num_elements + 10); i++) {
			this->ptr[i] = 0;
		}
	}

	~safe_ptr<T>() {
		kfree(this->ptr);
	}
	T operator*(void) {
		return *this->ptr;
	}
	T* operator&(void) {
		return this->ptr;
	}
	T* operator->(void) {
		return ptr;
	}

	T& operator[](size_t idx) {
		if (idx >= this->num_elements) {
			//Write to the kernel log that there was an invalid access attempt.
			//TODO: maybe throw a custom fault or signal for this, so that we don't trigger
			//the page fault interrupt handler?
			logfk(ERROR, "Index out of bounds: Addr: %x Index: %d", &(this->ptr[idx]), idx);
			//Options:
			//Just return a ref to the last element of the array and let the access go wild,
			//return this->ptr[this->num_elements - 1];
			//
			//Returning a static var. Effectively blackholes the write, but reads can still access the last written value
			//static T tmp;
			//return tmp;
			//
			//returning nothing here effectively causes a page fault on reads or writes
			return;
		}
		// return a ptr to the index
		return this->ptr[idx];
	}

	safe_ptr<T> operator+(uint64_t rhs) {
		if (rhs >= this->end) {
			return *this;
		}
		this->ptr + rhs;
		this->end - rhs;
		return this;
	}

	T* operator-(uint64_t rhs) { return nullptr; }
	T* operator*(uint64_t rhs) { return nullptr; }
	T* operator/(uint64_t rhs) { return nullptr; }

	uint64_t get_size() {
		return this->end;
	}
	T* get_raw() {
		return this->ptr;
	}
};
