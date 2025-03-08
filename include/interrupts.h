#pragma once

namespace Interrupts {
	void init();
	void init_idt();
	void load_idt();
	void idt_set_gate(uint8_t idx, void* base, uint8_t flags);
	void clear_idt();
	void test();
	void install_handler(int irq, void (*handler)(struct registers* r));
}
