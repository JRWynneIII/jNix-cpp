#pragma once

template<typename T>
class vIterator {
private:
	node<T>* cur;
public:
	vIterator(node<T>* n) : cur(n) {}

	T& operator*() { return cur->value; }

	vIterator& operator++() {
		this->cur = this->cur->get_next();
		return *this;
	}

	bool operator!=(const vIterator& rhs) const { 
		return this->cur != rhs.cur; 
	}
};

