
ifeq ($(shell uname -s),Darwin)
  LLVM_PREFIX := /opt/homebrew/opt/llvm/bin
else
  LLVM_PREFIX := /usr/bin
endif

CC      := $(LLVM_PREFIX)/clang
OBJCOPY := $(LLVM_PREFIX)/llvm-objcopy
CFLAGS := -std=c11 -O2 -g3 -Wall -Wextra --target=riscv32-unknown-elf \
          -fuse-ld=lld -fno-stack-protector -ffreestanding -nostdlib
# ユーザ空間ビルド時のみ USERSPACE を立て、syscall ベースの printf を有効化
USER_CFLAGS := $(CFLAGS) -DUSERSPACE

all: kernel.elf

shell.elf: shell.c user.c lib/common.c lib/common_stdio.c user.ld
	$(CC) $(USER_CFLAGS) -Wl,-Tuser.ld -Wl,-Map=shell.map -o $@ \
		shell.c user.c lib/common.c lib/common_stdio.c

shell.bin: shell.elf
	$(OBJCOPY) --set-section-flags .bss=alloc,contents -O binary $< $@

shell.bin.o: shell.bin
	$(OBJCOPY) -Ibinary -Oelf32-littleriscv $< $@

kernel.elf: kernel.c lib/common.c drivers/virtio.c filesystem/fat16.c shell.bin.o kernel.ld
	$(CC) $(CFLAGS) -Wl,-Tkernel.ld -Wl,-Map=kernel.map -o $@ \
		kernel.c lib/common.c drivers/virtio.c filesystem/fat16.c shell.bin.o

.PHONY: run clean
run: kernel.elf
	qemu-system-riscv32 -machine virt -bios default -nographic -serial mon:stdio --no-reboot \
		-drive id=drive0,file=fat16.img,format=raw,if=none \
		-device virtio-blk-device,drive=drive0,bus=virtio-mmio-bus.0 \
		-kernel $<

clean:
	rm -f shell.elf shell.bin shell.bin.o shell.map kernel.elf kernel.map
