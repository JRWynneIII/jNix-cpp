#pragma once

template<typename T>
class node {
	template<typename U>
	friend class vIterator;
private:
	T value;
	node<T>* next;
	node<T>* prev;
public:
	node() {
		this->next = nullptr;
		this->prev = nullptr;
	}
	
	node(T val) {
		this->next = nullptr;
		this->prev = nullptr;
		this->value = val;
	}
	
	node(node<T> const& rhs) {
		this->next = rhs->next;
		this->prev = rhs->prev;
		this->value = rhs->value;
	}
	
	~node() {
		//Remove from list and link neighbors
		if (this->next != nullptr)
			this->next->set_prev(this->prev);
		if (this->prev != nullptr)
			this->prev->set_next(this->next);
	}
	
	node<T>* get_next() { return this->next; }
	
	node<T>* get_prev() { return this->prev; }
	
	T get_value() { return this->value; }
	
	void set_next(node<T>* n) { this->next = n; }
	
	void set_prev(node<T>* p) { this->prev = p; }
	
	void set_value(T val) { this->value = val; }

//	node<T>();
//	node<T>(T val);
//	//Copy constructor
//	node<T>(node<T> const& rhs);
//	~node<T>();
//	node<T>* get_next();
//	node<T>* get_prev();
//	T get_value();
//	void set_next(node<T>* n);
//	void set_prev(node<T>* p);
//	void set_value(T val);
};

