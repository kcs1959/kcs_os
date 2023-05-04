# kcs_os
KCS original operating system

## setup development environment
### Install Rust toolchains

[https://www.rust-lang.org/tools/install](https://www.rust-lang.org/tools/install)
```
curl --proto '=https' --tlsv1.2 -sSf https://sh.rustup.rs | sh
```

### Update Rust toolchains
```
rustup update
```

### Install cargo-make
```
cargo install --force cargo-make
```

### Add target for UEFI application
```
rustup target add x86_64-unknown-uefi
```

### Install QEMU 
[https://www.qemu.org/download/#linux](https://www.qemu.org/download/#linux)

For Ubuntu
```
apt-get install qemu
```
For macOS
```
brew install qemu
```

### Create disk img for QEMU
```
cargo make img
```

### Run KCS OS
```
cargo make run
```