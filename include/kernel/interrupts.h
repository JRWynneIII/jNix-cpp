#pragma once

#define PIC_READ_IRR 0x0a	//Raised IRQ
#define PIC_READ_ISR 0x0b
#define PIC1_CMD 0x20
#define PIC2_CMD 0xA0
#define PIC1_DATA 0x21
#define PIC2_DATA 0xA1
#define PIC_INIT 0x11
#define SIG_EOI 0x20 //Signals the end of the interrupt


namespace Interrupts {
	void enable_nmi();
	void disable_nmi();
	void disable_all();
	uint8_t get_pic_mask(uint16_t port);
	void set_pic_mask(uint16_t port, uint8_t mask);
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
