#pragma once
#include<kernel.h>
#include<string.hpp>

class Command {
public:
	String Name;
	Command() {}
	//Command(String s) : Name(s) {}
	Command(char* s) : Name(String(s)) {}
	String get_name() { return this->Name; }
	virtual void run() {
		logfk(INFO, "%s\n", Name.cstring());
	}
};
