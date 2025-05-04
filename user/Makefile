.PHONY: all
all: crt0.o simple.elf loop.elf

crt0.o: crt0.S
	x86_64-elf-as -g crt0.S -o crt0.o

simple.elf: simple.S crt0.o
	x86_64-elf-as -g simple.S -o simple.o
	ld simple.o crt0.o -o simple.elf -e _start
	cp simple.elf ../sysroot/bin
	rm ../initrd.tar

loop.elf: loop.S crt0.o
	x86_64-elf-as -g loop.S -o loop.o
	ld loop.o crt0.o -o loop.elf -e _start
	cp loop.elf ../sysroot/bin
	rm ../initrd.tar

clean:
	rm *.o *.elf

