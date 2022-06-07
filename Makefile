arch ?= x86_64
kernel := build/kernel-$(arch).bin
img := build/os.img
iso := build/os-$(arch).iso

cc := ~/opt/cross/bin/x86_64-elf-gcc
warnings := -Wall -Werror
cc_flags := -c -g -mno-red-zone $(warnings)

linker_script := src/arch/$(arch)/linker.ld
grub_cfg := src/arch/$(arch)/grub.cfg
assembly_source_files := $(wildcard src/arch/$(arch)/*.asm)
assembly_object_files := $(patsubst src/arch/$(arch)/%.asm, \
	build/arch/$(arch)/%.o, $(assembly_source_files))
c_source_files := $(wildcard src/arch/$(arch)/*.c)
c_object_files := $(patsubst src/arch/$(arch)/%.c, \
	build/arch/$(arch)/%.o, $(c_source_files))
header_files := $(wildcard src/arc/$(arch)/*.h)

LOOPBACK := 30
LOOPBACK1 := 31

.PHONY: all clean run img

all: $(kernel)

clean:
	@rm -r build

run_iso: $(iso)

	@qemu-system-x86_64 -s -cdrom $(iso) -serial stdio

run: $(img)
	@qemu-system-x86_64 -s -drive format=raw,file=$(img) -serial stdio

img: $(img)

$(img): $(kernel) $(grub_cfg)
	@dd if=/dev/zero of=$(img) bs=512 count=32768                                    
	@parted $(img) mklabel msdos
	@parted $(img) mkpart primary fat32 2048s 30720s
	@parted $(img) set 1 boot on
	@sudo losetup /dev/loop$(LOOPBACK) $(img)
	@sudo losetup /dev/loop$(LOOPBACK1) $(img) -o 1048576
	@sudo mkdosfs -F32 -f 2 /dev/loop$(LOOPBACK1)
	@sudo mount /dev/loop$(LOOPBACK1) /mnt/fatgrub
	@mkdir -p build/img/boot/grub
	@cp $(kernel) build/img/boot/kernel.bin
	@cp $(grub_cfg) build/img/boot/grub
	@sudo cp -r build/img/boot /mnt/fatgrub
	@sudo grub-install --target=i386-pc --root-directory=/mnt/fatgrub --no-floppy \
		--modules="normal part_msdos ext2 multiboot" /dev/loop$(LOOPBACK)
	@sudo umount /mnt/fatgrub
	@sudo losetup -d /dev/loop$(LOOPBACK)
	@sudo losetup -d /dev/loop$(LOOPBACK1)

iso: $(iso)

$(iso): $(clean) $(kernel) $(grub_cfg)
	@mkdir -p build/isofiles/boot/grub
	@cp $(kernel) build/isofiles/boot/kernel.bin
	@cp $(grub_cfg) build/isofiles/boot/grub
	@grub-mkrescue -o $(iso) build/isofiles 2> /dev/null
	@rm -r build/isofiles

kernel: $(kernel)

$(kernel): $(assembly_object_files) $(linker_script) $(c_object_files)
	@ld -n -T $(linker_script) -o $(kernel) $(assembly_object_files) $(c_object_files)

build/arch/$(arch)/%.o : src/arch/$(arch)/%.c
	@$(cc) $(cc_flags) $< -o $@

# compile assembly files
build/arch/$(arch)/%.o: src/arch/$(arch)/%.asm
	@mkdir -p $(shell dirname $@)
	@nasm -felf64 $< -o $@
