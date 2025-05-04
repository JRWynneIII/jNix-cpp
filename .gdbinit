 file kernel/arch/x86_64/kernel.elf
 add-symbol-file sysroot/bin/loop.elf
 target remote localhost:1234
 break crt0.S:_start
 break main
 break isr14
 break isr13
