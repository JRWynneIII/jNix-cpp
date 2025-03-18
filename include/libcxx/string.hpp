#pragma once
#include<string.h>

class String {
private:
	char* cstr;
	uint64_t size; //strlen+1
	uint64_t len; //strlen
public:
	String() : cstr(nullptr), size(0), len(0) {}

	String(char* s) {
		this->len = strlen(s);
		this->size = this->len + 1;
		this->cstr = new char[this->size];
		memcpy(this->cstr, s, this->size);
	}

	//Copy constructor
	String(const String& s) { //: cstr(new char[s.size]), size(s.size), len(s.len) {
		this->len = strlen(s.cstr);
		this->size = this->len + 1;
		this->cstr = new char[this->size];
		memcpy(this->cstr, s.cstr, this->size);
		//memcpy(this->cstr, s.cstr, s.size);
	}

	~String() {
		if (this->cstr != nullptr)
			delete this->cstr;
		this->size = 0;
		this->len = 0;
	}

	uint64_t length() { return this->len; }

	char* cstring() { return this->cstr; }

	bool operator==(const String& rhs) { 
		return strcmp(this->cstr, rhs.cstr); 
	}

	bool operator!=(const String& rhs) { 
		bool ret = strcmp(this->cstr, rhs.cstr); 
		return !ret;
	}

	//String = String
	String& operator=(const String& s) {
		if (this->cstr != nullptr) delete this->cstr;

		this->cstr = new char[s.size];
		memcpy(this->cstr, s.cstring(), s.size);

		this->size = s.size;
		this->len = s.len;

		return *this;
	}

	// String = char*
	String& operator=(char* s) {
		if (this->cstr != nullptr) delete this->cstr;

		this->len = strlen(s);
		this->size = this->len + 1;

		this->cstr = new char[this->size];
		memcpy(this->cstr, s, this->size);

		return *this;
	}

	void trim() {
		// {"test", 4, 5}; [4] == '\0'; [5] == undef; [3] == 't'
		uint64_t last_idx = this->len-1;

		int idx = 0;
		for (idx = last_idx; idx >= 0; idx--) {
			if (this->cstr[idx] != '\n') break;
		}

		this->len = idx+1;
		this->size = this->len + 1;
		
		char* buf = new char[this->size];
		memcpy(buf, this->cstr, this->len);
		buf[this->size-1] = '\0';

		if (this->cstr != nullptr) delete this->cstr;

		this->cstr = buf;
	}

	String& operator+=(char* s) {
		//strlen(this) + strlen(s) + sizeof('\0')
		uint64_t buflen = this->len + strlen(s) + 1;
		char* buf = new char[buflen];

		memcpy(buf, this->cstr, this->len);
		memcpy(&(buf[this->len]), s, strlen(s));

		//Append null terminator
		buf[buflen-1] = '\0';
		
		if (this->cstr != nullptr) delete this->cstr;
		this->cstr = buf;
		this->len = buflen - 1;
		this->size = this->len + 1;
	}

	String& operator+=(char c) {
		// +1 for new char, +1 for \0
		uint64_t buflen = this->len + 1 + 1;

		char* buf = new char[buflen];
		memcpy(buf, this->cstr, this->len);
		buf[buflen-2] = c;
		buf[buflen-1] = '\0';

		if (this->cstr != nullptr) delete this->cstr;
		this->cstr = buf;
		this->len += 1;
		this->size += 1;
	}
};
