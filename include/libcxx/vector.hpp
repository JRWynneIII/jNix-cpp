#pragma once
#include<cstdint>
#include<cstddef>
#include<node.hpp>
#include<viterator.hpp>

template<typename T>
class vector {
private:
	node<T>* head; 
	node<T>* tail; 
	uint64_t size;
public:
	vector() {
		this->head = nullptr;
		this->tail = nullptr;
		this->size = 0;
	}
	
	~vector() {
		node<T>* cur = this->head;
		while(cur != nullptr) {
			node<T>* next = cur->get_next();
			delete cur;
			cur = next;
		}
	}
	
	vector(size_t size) {
	//	node<T>* cur = this->head;
	//	node<T>* prev = nullptr;
	//	cur = new node<T>();
	//	this->head = cur;
	//	for (int i=0 ; i < size; i++) {
	//		cur->set_next(new node<T>());
	//		cur->set_prev(prev);
	//		prev = cur;
	//		cur = cur->get_next();
	//	}
	//	this->size = size;
	//	this->tail = prev;
	}

	vIterator<T> begin() { return vIterator<T>(this->head); }
	vIterator<T> end() { return vIterator<T>(nullptr); }
	
	//template<typename T>
	//vector::vector<T>(vector<T> const& rhs): head(rhs.head), tail(rhs.tail), size(rhs.size) {
	////TODO: Might need to loop through and delete the whole list for the old `this`
	//}
	
	void push_back(T val) {
		if (tail != nullptr) {
			node<T>* old_tail = this->tail;
			node<T>* new_node = new node<T>(val);
			new_node->set_prev(this->tail);
			this->tail->set_next(new_node);
			this->tail = new_node;
		} else {
			this->head = new node<T>(val);
			this->tail = this->head;
		}
		this->size++;
	}

	void push_back_no_heap(T val) {
		if (tail != nullptr) {
			node<T>* old_tail = this->tail;
			node<T> new_node = node<T>(val);
			new_node.set_prev(this->tail);
			this->tail->set_next(&new_node);
			this->tail = &new_node;
		} else {
			node<T> no = node<T>(val);
			this->head = &no;
			this->tail = this->head;
		}
		this->size++;
	}
	
	T pop_head() {
		T ret = this->head->get_value();
		node<T>* new_head = head->get_next();

		if (this->head == this->tail) {
			this->tail = nullptr;
			new_head = nullptr;
		}

		delete this->head;
		this->head = new_head;
		this->size--;
		return ret;
	}
	
	T peek_head() {
		return this->head->get_value();
	}

	T pop_tail() {
		T ret = this->tail->get_value();
		node<T>* new_tail = tail->get_prev();
		delete this->tail;
		this->tail = new_tail;
		return ret;
	}
	
	T peek_tail() {
		return this->tail->get_value();
	}
	
//	T& operator[](size_t idx) {
//		uint64_t count = 0;
//		node<T> cur = this->head;
//		while(count != idx && count < this->size()) {
//			count++;
//			cur = cur->get_next();
//		}
//		// TODO: may need to write a new node:: method to return a reference instead?
//		return cur->get_value();
//	}
	
	vector<T>& operator=(nullptr_t const& rhs) {
		//Free up the old list
		node<T>* cur = this->head;
		while (cur != nullptr) {
			node<T>* next = cur->get_next();
			delete cur;
			cur = next;
		}
	
		this->tail = nullptr;
		this->head = nullptr;
		this->size = 0;
	}
	
	vector<T>& operator=(vector<T> const& rhs) {
		//Free up the old list
		node<T>* cur = this->head;
		while (cur != nullptr) {
			node<T>* next = cur->get_next();
			delete cur;
			cur = next;
		}
	
		this->tail = rhs.get_tail();
		this->head = rhs.get_head();
		this->size = rhs.length();
	}
	
	bool operator!=(nullptr_t rhs) {
		if (this->head != nullptr && this->tail != nullptr && this->size != 0) return false;
		return true;
	}
	
	bool operator!=(vector<T> const& rhs) {
		node<T>* cur = this->head;
		uint64_t idx = 0;
		while (cur != nullptr) {
			if (cur->get_value() != rhs[idx]) {
				break;
			}
			idx++;
		}
		if (idx != (this->size - 1))
			return false;
		return true;
	}

	void del(uint64_t idx) {
		uint64_t count = 0;
		node<T>* cur = this->head;
		while(cur != nullptr && count != idx && count < idx) {
			if (count == idx) break;
			cur = cur->get_next();
			count++;
		}
		if (cur != nullptr) {
			delete cur;
			this->size--;
		}
		if (this->size == 0) {
			this->head = nullptr;
			this->tail = nullptr;
		}
	}

	void del_by_value(T val) {
		node<T>* cur = this->head;
		while(cur != nullptr) {
			if (val == cur->get_value()) break;
			cur = cur->get_next();
		}
		if (cur != nullptr) {
			if (cur == this->head) {
				this->head = cur->get_next();
			}
			if (cur == this->tail) {
				this->tail = cur->get_prev();
			}
			delete cur;
			this->size--;
		}
		if (this->size == 0) {
			this->head = nullptr;
			this->tail = nullptr;
		}
	}
	
	void set(uint64_t idx, T value) {
		uint64_t count = 0;
		node<T>* cur = this->head;
		while(cur != nullptr && count != idx && count < idx) {
			if (count == idx) break;
			cur = cur->get_next();
			count++;
		}
		cur->set_value(value);
	}
	
	T at(uint64_t idx) {
		uint64_t count = 0;
		node<T>* cur = this->head;
		while(cur != nullptr && count != idx && count < idx) {
			if (count == idx) break;
			cur = cur->get_next();
			count++;
		}
		return cur->get_value();
	}
	
	uint64_t length() { return this->size; }
	node<T>* get_head() { return this->head; }
	node<T>* get_tail() { return this->tail; }
	vector<T>(vector<T> const& rhs): head(rhs.head), tail(rhs.tail), size(rhs.size) {};
};

