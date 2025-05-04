#pragma once
#include<kernel/gdt.hpp>

namespace GDT {
	gdt_t& gdt();
	void init();
	void dump();
}
