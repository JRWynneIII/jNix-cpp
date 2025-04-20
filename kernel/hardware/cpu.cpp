#include<kernel.h>
#include<kernel/processor.hpp>
#include<kernel/processor/x86_64.hpp>

namespace Hardware {
	processor_t& CPU() {
		static x86_64 cpu = x86_64();
		return cpu;
	}
}
