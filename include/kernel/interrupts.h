#pragma once

namespace Interrupts {
	void mask_irq(uint64_t irq);
	void unmask_irq(uint64_t irq);
	void write_mask(uint64_t irq, uint8_t mask);
	uint8_t get_mask(uint64_t irq);
	void init();
	void init_idt();
	void load_idt();
	void idt_set_gate(uint8_t idx, void* base, uint8_t flags);
	void clear_idt();
	void test();
	//void install_handler(int irq, void (*handler)(struct registers* r));
	void install_handler(int, void*);
}
