#pragma once
#include<kernel/ptr.hpp>

namespace Streams {
	//TODO: I WOULD KILL FOR A VECTOR
	//ptr_t<char> stdin = new ptr_t<char>(128*sizeof(char));
	//extern ptr_t<uint8_t> stdin;
	vector<uint8_t>& stdin();
}

