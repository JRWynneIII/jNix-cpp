#include<kernel/ptr.hpp>

namespace Streams {
	ptr_t<uint8_t> stdin = ptr_t<uint8_t>(128*sizeof(uint8_t));
}
