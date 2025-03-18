#include<vector.hpp>

namespace Streams {
	vector<uint8_t>& stdin() {
		static vector<uint8_t>* s = new vector<uint8_t>();
		return *s;
	}
	//ptr_t<uint8_t> stdin = ptr_t<uint8_t>(128*sizeof(uint8_t));
}
