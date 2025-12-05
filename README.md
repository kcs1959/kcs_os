# KCS OS

```bash
curl -LO https://github.com/qemu/qemu/raw/v8.0.4/pc-bios/opensbi-riscv32-generic-fw_dynamic.bin
qemu-img create -f raw fat16.img 16M
./run.sh
```
