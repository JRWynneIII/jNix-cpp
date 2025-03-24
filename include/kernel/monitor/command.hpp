#pragma once
#include<kernel.h>
#include<vector.hpp>

class Command {
public:
	char* Name;
	Command() {}
	Command(char* s) : Name(s) {}
	char* get_name() { return this->Name; }
	virtual void run(vector<char*>* args) {
		logfk(INFO, "%s\n", Name);
	}
};
