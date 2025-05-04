#pragma once
#include<kernel/process/elf.hpp>

extern "C" void exit_to_ring3(uint64_t, uint64_t);
extern "C" void swap_to_context(cpu_context_t*, uint64_t);
extern "C" void swap_to_kernel_context(cpu_context_t*, uint64_t);
extern "C" void save_cpu_context(cpu_context_t*);
extern "C" void enable_sysret();
