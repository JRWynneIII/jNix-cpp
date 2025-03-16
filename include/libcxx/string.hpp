#pragma once
#include<string.h>

class String {
private:
	char* cstr;
	uint64_t size;
public:
	String() {
		cstr = new char[1];
		cstr[0] = '\0';
		size = 0;
	}

	String(char* s) : cstr(s), size(strlen(s)) {}
//	String(const String& s) {
//		//delete this->cstr;
//		if (this->cstr != nullptr) delete this->cstr;
//		this->cstr = new char[s.length()+1];
//		this->size = s.length();
//		memcpy(this->cstr, s.cstring(), s.length()+1);
//	}

	uint64_t length() { return this->size; }

	char* cstring() { return this->cstr; }

	bool operator==(const String& rhs) { return strcmp(this->cstr, rhs.cstring()); }
	bool operator!=(const String& rhs) { return !strcmp(this->cstr, rhs.cstring()); }

//	String& operator=(String s) {
//		if (this->cstr != nullptr) {
//			delete this->cstr;
//		}
//		this->cstr = new char[s.length()+1];
//		memcpy(this->cstr, s.cstring(), s.length()+1);
//		this->size = s.length();
//		return *this;
//	}

	String& operator=(char* s) {
		if (this->cstr != nullptr) delete this->cstr;
		this->cstr = new char[strlen(s)+1];
		memcpy(this->cstr, s, strlen(s)+1);
		this->size = strlen(s);
		return *this;
	}

	void replace(char old, char newchar) {
		char* cur = this->cstr;
		while(*cur != '\0') {
			if (*cur == old) *cur = newchar;
			cur++;
		}

	}

	String& operator+=(char* s) {
		uint64_t buflen = this->size + strlen(s) + 1;
		char* buf = new char[buflen];
		int i = 0;
		char* cur = this->cstr;
		memcpy(buf, this->cstr, strlen(this->cstr));
		memcpy(&(buf[strlen(this->cstr)]), s, strlen(s)+1);
		buf[buflen-1] = '\0';
		//delete this->cstr;
		if (this->cstr != nullptr) delete this->cstr;
		this->cstr = buf;
		this->size = strlen(buf);
	}

	String& operator+=(char c) {
		// +1 for new char, +1 for \0
		uint64_t buflen = this->size + 2;
		char* buf = new char[buflen];
		int i = 0;
		char* cur = this->cstr;
		while(*cur != '\0') {
			buf[i] = *cur;
			i++;
			cur++;
		}

		buf[buflen-2] = c;
		buf[buflen-1] = '\0';
		//delete this->cstr;
		if (this->cstr != nullptr) delete this->cstr;
		this->cstr = buf;
		this->size = strlen(buf);
	}
};
