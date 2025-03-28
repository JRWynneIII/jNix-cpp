.PHONY: all
all: jnix.iso

.PHONY: all-hdd
all-hdd: jnix.hdd

.PHONY: run
run: jnix.iso
	qemu-system-x86_64 -monitor stdio -rtc base=localtime,clock=vm -no-shutdown -no-reboot -M q35 -m 2G -cdrom jnix.iso -boot d 

.PHONY: debug
debug: jnix.iso
	qemu-system-x86_64 -s -S -monitor stdio -no-shutdown -d int -no-reboot -M q35 -m 2G -cdrom jnix.iso -boot d

.PHONY: run-uefi
run-uefi: ovmf-x64 jnix.iso
	qemu-system-x86_64 -M q35 -m 2G -bios ovmf-x64/OVMF.fd -cdrom jnix.iso -boot d

.PHONY: run-hdd
run-hdd: jnix.hdd
	qemu-system-x86_64 -M q35 -m 2G -hda jnix.hdd

.PHONY: run-hdd-uefi
run-hdd-uefi: ovmf-x64 jnix.hdd
	qemu-system-x86_64 -M q35 -m 2G -bios ovmf-x64/OVMF.fd -hda jnix.hdd

ovmf-x64:
	mkdir -p ovmf-x64
	cd ovmf-x64 && curl -o OVMF-X64.zip https://efi.akeo.ie/OVMF/OVMF-X64.zip && 7z x OVMF-X64.zip

flanterm:
	git clone https://github.com/mintsuki/flanterm kernel/flanterm || true

limine:
	git clone https://github.com/limine-bootloader/limine.git --branch=v8.x-binary --depth=1
	make -C limine

#deps: libc/libk.a flanterm libc/libc.a
deps: flanterm
	git clone https://github.com/osdev0/freestnd-c-hdrs.git kernel/freestanding-c || true
	git clone https://github.com/osdev0/freestnd-cxx-hdrs.git kernel/freestanding-cxx || true
	wget https://raw.githubusercontent.com/limine-bootloader/limine/refs/heads/v8.x-binary/limine.h -O include/

#libc/libk.a:
#	$(MAKE) -C libc
#
#libc/libc.a:
#	$(MAKE) -C libc

.PHONY: kernel
kernel: deps
	$(MAKE) -C kernel

initrd.tar:
	tar cf initrd.tar sysroot

#jnix.iso: deps limine libc/libk.a kernel
jnix.iso: deps limine kernel initrd.tar
	rm -rf iso_root
	mkdir -p iso_root/boot
	cp -v kernel/arch/x86_64//kernel.elf iso_root/boot/
	cp -v initrd.tar iso_root/boot/
	mkdir -p iso_root/boot/limine
	cp -v limine.conf iso_root/boot/limine/
	mkdir -p iso_root/EFI/BOOT
	cp -v limine/limine-bios.sys limine/limine-bios-cd.bin limine/limine-uefi-cd.bin iso_root/boot/limine/
	cp -v limine/BOOTX64.EFI iso_root/EFI/BOOT/
	cp -v limine/BOOTIA32.EFI iso_root/EFI/BOOT/
	#rm -rf iso_root
	#mkdir -p iso_root/boot/limine
	#cp kernel/arch/x86_64/kernel.elf \
	#	limine.cfg limine/limine-bios.sys limine/limine-cd.bin limine/limine-cd-efi.bin iso_root/
	xorriso -as mkisofs -R -r -J -b boot/limine/limine-bios-cd.bin \
		-no-emul-boot -boot-load-size 4 -boot-info-table -hfsplus \
		-apm-block-size 2048 --efi-boot boot/limine/limine-uefi-cd.bin \
		-efi-boot-part --efi-boot-image --protective-msdos-label \
		iso_root -o jnix.iso
	limine/limine bios-install jnix.iso
	rm -rf iso_root

# TODO: see: https://github.com/limine-bootloader/limine-cxx-template/blob/trunk/GNUmakefile
#jnix.hdd: deps limine  kernel
#	rm -f jnix.hdd
#	dd if=/dev/zero bs=1M count=0 seek=64 of=jnix.hdd
#	parted -s jnix.hdd mklabel gpt
#	parted -s jnix.hdd mkpart ESP fat32 2048s 100%
#	parted -s jnix.hdd set 1 esp on
#	limine/limine-deploy jnix.hdd
#	sudo losetup -Pf --show jnix.hdd >loopback_dev
#	sudo mkfs.fat -F 32 `cat loopback_dev`p1
#	mkdir -p img_mount
#	sudo mount `cat loopback_dev`p1 img_mount
#	sudo mkdir -p img_mount/EFI/BOOT
#	sudo cp -v kernel/kernel.elf limine.cfg limine/limine.sys img_mount/
#	sudo cp -v limine/BOOTX64.EFI img_mount/EFI/BOOT/
#	sync
#	sudo umount img_mount
#	sudo losetup -d `cat loopback_dev`
#	rm -rf loopback_dev img_mount

.PHONY: clean
clean:
	rm -rf iso_root jnix.iso jnix.hdd
	$(MAKE) -C kernel clean

.PHONY: distclean
distclean: clean
	rm -rf limine ovmf-x64 kernel/freestanding-c kernel/freestanding-cxx include/limine.h kernel/flanterm
	$(MAKE) -C kernel distclean
