
ifeq ($(shell uname -s),Darwin)
  LLVM_PREFIX := /opt/homebrew/opt/llvm/bin
else
  LLVM_PREFIX := /usr/bin
endif

CC      := $(LLVM_PREFIX)/clang
OBJCOPY := $(LLVM_PREFIX)/llvm-objcopy
CFLAGS := -std=c11 -O2 -g3 -Wall -Wextra --target=riscv32-unknown-elf \
          -fuse-ld=lld -fno-stack-protector -ffreestanding -nostdlib \
          -I. -Ikernel -Iuser -Icommon

all: kernel.elf fat16.img ## Build the kernel ELF file and shell binary

help: ## Show this help message
	@grep -E '^[a-zA-Z0-9_-]+:.*?## .*$$' $(MAKEFILE_LIST) | awk 'BEGIN {FS = ":.*?## "}; {printf "\033[36m%-20s\033[0m %s\n", $$1, $$2}'

mount: ## Mount the FAT16 image file to ./fat directory
	@if [ ! -f fat16.img ]; then \
		echo "FAT16 image file 'fat16.img' does not exist."; \
		exit 1; \
	fi
	@if [ ! -d fat ]; then \
		mkdir fat; \
	fi
	sudo mount -o loop,uid=$$(id -u),gid=$$(id -g) -t vfat fat16.img ./fat
	

unmount: ## Unmount the FAT16 image file and remove the mount directory
	sudo umount ./fat
	rm -rf fat

fat16: fat16.img ## Create FAT16 filesystem image

fat16.img:
	@if [ ! -f fat16.img ]; then \
		qemu-img create -f raw fat16.img 16M; \
	fi

shell.elf: user/shell.c user/usys.c common/common.c user/user.ld \
           common/common_types.h common/common.h
	$(CC) $(CFLAGS) -Wl,-Tuser/user.ld -Wl,-Map=shell.map -o $@ \
		user/shell.c user/usys.c common/common.c 

shell.bin: shell.elf
	$(OBJCOPY) --set-section-flags .bss=alloc,contents -O binary $< $@

shell.bin.o: shell.bin
	$(OBJCOPY) -Ibinary -Oelf32-littleriscv $< $@

kernel.elf: kernel/kernel.c kernel/virtio.c kernel/fat16.c kernel/kernel.ld \
            shell.bin.o common/common.c common/common_types.h common/common.h
	$(CC) $(CFLAGS) -Wl,-Tkernel/kernel.ld -Wl,-Map=kernel.map -o $@ \
		kernel/kernel.c kernel/virtio.c kernel/fat16.c common/common.c \
		shell.bin.o

.PHONY: run clean help mount unmount
run: kernel.elf ## Run the kernel in QEMU
	qemu-system-riscv32 -machine virt -bios default -nographic -serial mon:stdio --no-reboot \
		-drive id=drive0,file=fat16.img,format=raw,if=none \
		-device virtio-blk-device,drive=drive0,bus=virtio-mmio-bus.0 \
		-kernel $<

clean: ## Clean up build artifacts
	rm -f shell.elf shell.bin shell.bin.o shell.map kernel.elf kernel.map
