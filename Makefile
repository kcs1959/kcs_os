
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

all: kernel.elf

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

.PHONY: run clean
run: kernel.elf
	qemu-system-riscv32 -machine virt -bios default -nographic -serial mon:stdio --no-reboot \
		-drive id=drive0,file=fat16.img,format=raw,if=none \
		-device virtio-blk-device,drive=drive0,bus=virtio-mmio-bus.0 \
		-kernel $<

clean:
	rm -f shell.elf shell.bin shell.bin.o shell.map kernel.elf kernel.map
