#pragma once
#include<cstddef>
#include<unwind.h>
#include<kernel.h>

namespace Memory {
	namespace Allocation {
		void kfree(void* vaddr);
		void* kalloc(uint64_t objsize, uint64_t num);
	}
}

template<typename T>
class ptr_t {
private:
	T* ptr;
	uint64_t end;
	uint64_t num_elements;
	uint64_t cur_idx = 0;
public:
	ptr_t<T>() {
		this->ptr = (T*)Memory::Allocation::kalloc(sizeof(T), 1);
		this->end = sizeof(T);
		this->num_elements = 1;
	}

	ptr_t<T>(size_t size) {
		T* p = (T*)Memory::Allocation::kalloc(size, 1);
		this->ptr = p;
		this->end = size;
		this->num_elements = size / sizeof(T);
		//for (int i = 0; i < (this->num_elements + 10); i++) {
		//	this->ptr[i] = 0;
		//}
	}

	ptr_t<T>(T* p, size_t size) {
		this->ptr = p;
		this->end = size;
		this->num_elements = size / sizeof(T);
		//for (int i = 0; i < (this->num_elements + 10); i++) {
		//	this->ptr[i] = 0;
		//}
	}

	ptr_t<T>(ptr_t<T> const& rhs): ptr(rhs.ptr), end(rhs.end), num_elements(rhs.num_elements) {
		this->ptr = rhs.ptr;
		this->end = rhs.end;
		this->num_elements = rhs.num_elements;
	}

	ptr_t<T>& operator=(nullptr_t const& rhs) {
		this->ptr = nullptr;
		this->end = 0;
		this->num_elements = 0;
	}

	ptr_t<T>& operator=(ptr_t<T> const& rhs) {
		this->ptr = rhs.ptr;
		this->end = rhs.end;
		this->num_elements = rhs.num_elements;
	}

	bool operator!=(nullptr_t rhs) {
		if (this->ptr == nullptr) return false;
		return true;
	}

	~ptr_t<T>() {
		Memory::Allocation::kfree(this->ptr);
	}

	// Use this method when your ptr_t is itself a ptr otherwise, the [] operator should work
	void set(uint64_t idx, T value) {
		if (idx >= this->num_elements) {
			logfk(ERROR, "Index out of bounds: Addr: %x Index: %d", &(this->ptr[idx]), idx);
			return;
		}
		this->ptr[idx] = value;
	}

	T pop() {
		T val = this->ptr[0];
		for (int i = 1; i < this->get_num_elements(); i++)
			this->ptr[i-1] = this->at(i);
		this->cur_idx--;
		return val;
	}

	// Use this method when your ptr_t is itself a ptr otherwise, the [] operator should work
	// I wonder if you could use this method to set and get?
	T& at(uint64_t idx) {
		//TODO: do bounds check
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

	void append(T val) {
		this->at(cur_idx) = val;
		cur_idx++;
	}

	T* operator*(void) {
		return this->ptr;
	}

	T* operator&(void) {
		return this->ptr;
	}

	T* operator->(void) {
		return ptr;
	}

	T& operator[](size_t idx) {
		return this->at(idx);
	}

	ptr_t<T> operator+(uint64_t rhs) {
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
	uint64_t get_num_elements() {
		return this->num_elements;
	}
	T* get_raw() {
		return this->ptr;
	}
};
