set -xue

CC=/opt/homebrew/opt/llvm/bin/clang

CFLAGS="-std=c11 -O2 -g3 -Wall -Wextra --target=riscv32-unknown-elf -fuse-ld=lld -fno-stack-protector -ffreestanding -nostdlib"

OBJCOPY=/opt/homebrew/opt/llvm/bin/llvm-objcopy

# シェルをビルド
$CC $CFLAGS -Wl,-Tuser.ld -Wl,-Map=shell.map -o shell.elf shell.c user.c common.c
$OBJCOPY --set-section-flags .bss=alloc,contents -O binary shell.elf shell.bin
$OBJCOPY -Ibinary -Oelf32-littleriscv shell.bin shell.bin.o

# カーネルをビルド
$CC $CFLAGS -Wl,-Tkernel.ld -Wl,-Map=kernel.map -o kernel.elf \
    kernel.c common.c shell.bin.o

qemu-system-riscv32 -machine virt -bios default -nographic -serial mon:stdio --no-reboot \
    -kernel kernel.elf



<< COMMENTOUT
アプリケーションのビルドの流れ
shell.c（アプリケーション）
    ↓ コンパイル
shell.o（オブジェクトファイル）
    ↓ リンク
shell.elf（機械語）
    ↓ カーネルのelfローダ
実行

今回は・・・
ユーザーアプリをカーネルに内蔵するときのアプリケーションビルド
shell.c（Cコード）
    ↓ コンパイル by clang
shell.elf（ELF、まだ単体で実行可能）
    ↓ llvm-objcopy -O binary
shell.bin（機械語だけの生バイナリ）
    ↓ llvm-objcopy -I binary -O elf32-littleriscv
shell.bin.o（オブジェクトファイル化）
    ↓ カーネルのビルド時にリンク
kernel.elf（カーネル + shell をまとめた実行可能 ELF）
COMMENTOUT

<<COMMENTOUT
前提：
-I(input), -O(output)
大文字：形式
小文字：フィアル名やパス

１、ユーザープログラムをelfファイルにする
$CC $CFLAGS -Wl,-Tuser.ld -Wl,-Map=shell.map -o shell.elf shell.c user.c common.c
$CC はClangのフルパス
$CFLAGS は共通コンパイルフラグ
-Wlオプション（以下参照）、-o出力ファイル、コンパイル対象はshell.c, user.c, common.c


２、elfファイルを生のバイナリにする
$OBJCOPY --set-section-flags .bss=alloc,contents -O binary shell.elf shell.bin
llvm-objcopyパス .bssも出力　　　　　　　　　　　　　　出力はバイト列　入力　　　出力

3、生バイナリ→オブジェクトファイル
$OBJCOPY -Ibinary -Oelf32-littleriscv shell.bin shell.bin.o


COMMENTOUT