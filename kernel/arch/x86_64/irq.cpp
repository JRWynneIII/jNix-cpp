#include<stdbool.h>
#include<stddef.h>
#include<stdint.h>
#include<stdlib.h>
#include<kernel.h>
#include<kernel/port.h>
#include<kernel/panic.h>
#include<kernel/interrupts.h>

extern "C" void irq0();
extern "C" void irq1();
extern "C" void irq2();
extern "C" void irq8();

extern "C" void isr0();
extern "C" void isr1();
extern "C" void isr2();
extern "C" void isr3();
extern "C" void isr4();
extern "C" void isr5();
extern "C" void isr6();
extern "C" void isr7();
extern "C" void isr8();
extern "C" void isr9();
extern "C" void isr10();
extern "C" void isr11();
extern "C" void isr12();
extern "C" void isr13();
extern "C" void isr14();
extern "C" void isr15();
extern "C" void isr16();
extern "C" void isr17();
extern "C" void isr18();
extern "C" void isr19();
extern "C" void isr20();
extern "C" void isr21();
extern "C" void isr22();
extern "C" void isr23();
extern "C" void isr24();
extern "C" void isr25();
extern "C" void isr26();
extern "C" void isr27();
extern "C" void isr28();
extern "C" void isr29();
extern "C" void isr30();
extern "C" void isr31();

extern char* exception_messages[] =
{
	"Division By Zero",
	"Debug Exception",
 	"Non Maskable Interrupt Exception",
	"Breakpoint Exception",
	"Into Detected Overflow Exception",
	"Out of Bounds Exception",
	"Invalid Opcode Exception",
	"No Coprocessor Exception",
	"Double Fault",
	"Coprocessor Segment Overrun",
	"Bad TSS",
	"Segment not Present",
	"Stack Fault",
	"General Protection Fault",
	"Page Fault",
	"Unknown Interrupt Exception",
	"Coprocessor Fault",
	"Alignment Check Exception",
	"Machine Check Exception",
	"Reserved",
	"Reserved",
	"Reserved",
	"Reserved",
	"Reserved",
	"Reserved",
	"Reserved",
	"Reserved",
	"Reserved",
	"Reserved",
	"Reserved",
	"Reserved",
	"Reserved"
};


namespace Interrupts {
	void* irq_handlers[16] = {
		0,0,0,0,0,0,0,0,
		0,0,0,0,0,0,0,0
	};

	//void install_handler(int irq, void (*handler)(struct registers* r)) {
	void install_handler(int irq, void* handler) { 
		irq_handlers[irq] = handler;
	}

	void uninstall_handler(int irq) {
		irq_handlers[irq] = 0;
	}

	extern "C" void irq_handler(struct registers *r) {
		// Void function pointer
		void (*handler)(struct registers *r);

		// Get the irq handler for the interrupt number and run if exists
		handler = irq_handlers[r->int_no - 32];
		if (handler != 0) {
			handler(r);
		}

		// End the interrupt
		if ((r->int_no-32) >= 8) {
			outportb((uint16_t)PIC2_CMD, (uint8_t)SIG_EOI);
		}
		outportb(PIC1_CMD, SIG_EOI);
	}

	static inline void io_wait(void) {
		/* TODO: This is probably fragile. */
		asm volatile("jmp 1f\n\t"
		             "1:jmp 2f\n\t"
		             "2:" );
	}

	void init_pic() {
		//Save current PIC masks
		uint8_t m1, m2;
		m1 = inportb(PIC1_DATA);
		m2 = inportb(PIC2_DATA);

		//Inialize both pics
		outportb(PIC1_CMD, PIC_INIT);
		outportb(PIC2_CMD, PIC_INIT);
		io_wait();

		// What is 32, 40?
		outportb(PIC1_DATA, 32);
		outportb(PIC2_DATA, 40);
		io_wait();

		//Inform master of the adjacent PIC
		outportb(PIC1_DATA, 4);
		outportb(PIC2_DATA, 2);
		io_wait();

		// Put PICs into 8086 mode
		outportb(PIC1_DATA, 0x01);
		outportb(PIC2_DATA, 0x01);
		io_wait();

		//Restore masks
		outportb(PIC1_DATA, m1);
		outportb(PIC1_DATA, m2);

		//Mask all
		outportb(PIC1_DATA,0xff);
		outportb(PIC2_DATA,0xff);
	}

	void disable_all() {
		outportb(PIC1_DATA, 0xFF);
		outportb(PIC2_DATA, 0xFF);
	}

	void set_pic_mask(uint16_t port, uint8_t mask) {
		__asm__ __volatile__("cli");
		outportb(port, mask);
		__asm__ __volatile__("sti");
	}

	uint8_t get_pic_mask(uint16_t port) {
		return inportb(port);
	}

	uint8_t get_mask(uint64_t irq) {
		//Save current PIC masks
		//uint16_t port = PIC1_DATA;
		if (irq >= 8) return inportb(PIC2_DATA); // port = PIC2_DATA;
		return inportb(PIC1_DATA);
	}

	void disable_nmi() {
		uint8_t cur = inportb(0x70);
		outportb(0x70, (cur | 0x80));
		//Need to read p 70 or else ctrlr may be in a weird state
		inportb(0x71);
	}

	void enable_nmi() {
		uint8_t cur = inportb(0x70);
		outportb(0x70, (cur & 0x7F));
		//Need to read p 70 or else ctrlr may be in a weird state
		inportb(0x71);
	}

	void write_mask(uint64_t irq, uint8_t mask) {
		//Save current PIC masks
		__asm__ __volatile__("cli");

		if (irq >= 8) { 
			outportb(PIC2_DATA, mask);
		} else {
			outportb(PIC1_DATA, mask);
		}
		__asm__ __volatile__("sti");
	}

	void mask_irq(uint64_t irq) {
		uint8_t cur_mask = get_mask(irq);
		if (irq >= 8 ) irq -= 8;
		write_mask(irq, (cur_mask | (1<<irq)));
	}

	void unmask_irq(uint64_t irq) {
		uint8_t cur_mask = get_mask(irq);
		uint8_t line = irq;
		if (irq >= 8 ) line -= 8;
		write_mask(irq, (cur_mask & ~(1<<line)));
	}

	void init() {
		logfk(KERNEL, "Initializing IDT");
		init_idt();
		printk("....DONE\n");

		//clear_idt();
		idt_set_gate(0, isr0, 0xE);
		idt_set_gate(1, isr1, 0xF);
		idt_set_gate(2, isr2, 0xE);
		idt_set_gate(3, isr3, 0xF);
		idt_set_gate(4, isr4, 0xF);
		idt_set_gate(5, isr5, 0xE);
		idt_set_gate(6, isr6, 0xE);
		idt_set_gate(7, isr7, 0xE);
		idt_set_gate(8, isr8, 0xE);
		idt_set_gate(9, isr9, 0xE);
		idt_set_gate(10, isr10, 0xE);
		idt_set_gate(11, isr11, 0xE);
		idt_set_gate(12, isr12, 0xE);
		idt_set_gate(13, isr13, 0xE);
		idt_set_gate(14, isr14, 0xE);
		idt_set_gate(15, isr15, 0xE);
		idt_set_gate(16, isr16, 0xE);
		idt_set_gate(17, isr17, 0xE);
		idt_set_gate(18, isr18, 0xE);
		idt_set_gate(19, isr19, 0xE);
		idt_set_gate(20, isr20, 0xE);
		idt_set_gate(21, isr21, 0xE);
		idt_set_gate(22, isr22, 0xE);
		idt_set_gate(23, isr23, 0xE);
		idt_set_gate(24, isr24, 0xE);
		idt_set_gate(25, isr25, 0xE);
		idt_set_gate(26, isr26, 0xE);
		idt_set_gate(27, isr27, 0xE);
		idt_set_gate(28, isr28, 0xE);
		idt_set_gate(29, isr29, 0xE);
		idt_set_gate(30, isr30, 0xE);
		idt_set_gate(31, isr31, 0xE);

		idt_set_gate(32, irq0, 0xE);
		idt_set_gate(33, irq1, 0xE);
		idt_set_gate(34, irq2, 0xE);
		idt_set_gate(34, irq8, 0xE);

		logfk(KERNEL, "Loading IDT");
		load_idt();
		printk("....DONE\n");
		logfk(KERNEL, "Initializing PIC");
		init_pic();
		printk("....DONE\n");
		//Enable interrupts
		logfk(KERNEL, "Enabling interrupts");
		__asm__ __volatile__("sti");
		printk("....DONE\n");
	}


	extern "C" void fault_handler(struct registers *r) {
		if (r->int_no < 32) {
			// Since we're just in kernel-land 100% of the time now, lets just kernel panic
			kpanic(r);
			// TODO: Modify this to account for user-space faults; kill process if faulted, basically
		}
	}


}




