__attribute__((naked)) __attribute__((aligned(4))) void kernel_entry(void) {
  __asm__ __volatile__("csrw sscratch, sp\n"
                       "addi sp, sp, -4 * 31\n"
                       "sw ra,  4 * 0(sp)\n"
                       "sw gp,  4 * 1(sp)\n"
                       "sw tp,  4 * 2(sp)\n"
                       "sw t0,  4 * 3(sp)\n"
                       "sw t1,  4 * 4(sp)\n"
                       "sw t2,  4 * 5(sp)\n"
                       "sw t3,  4 * 6(sp)\n"
                       "sw t4,  4 * 7(sp)\n"
                       "sw t5,  4 * 8(sp)\n"
                       "sw t6,  4 * 9(sp)\n"
                       "sw a0,  4 * 10(sp)\n"
                       "sw a1,  4 * 11(sp)\n"
                       "sw a2,  4 * 12(sp)\n"
                       "sw a3,  4 * 13(sp)\n"
                       "sw a4,  4 * 14(sp)\n"
                       "sw a5,  4 * 15(sp)\n"
                       "sw a6,  4 * 16(sp)\n"
                       "sw a7,  4 * 17(sp)\n"
                       "sw s0,  4 * 18(sp)\n"
                       "sw s1,  4 * 19(sp)\n"
                       "sw s2,  4 * 20(sp)\n"
                       "sw s3,  4 * 21(sp)\n"
                       "sw s4,  4 * 22(sp)\n"
                       "sw s5,  4 * 23(sp)\n"
                       "sw s6,  4 * 24(sp)\n"
                       "sw s7,  4 * 25(sp)\n"
                       "sw s8,  4 * 26(sp)\n"
                       "sw s9,  4 * 27(sp)\n"
                       "sw s10, 4 * 28(sp)\n"
                       "sw s11, 4 * 29(sp)\n"

                       "csrr a0, sscratch\n"
                       "sw a0, 4 * 30(sp)\n"

                       "mv a0, sp\n"
                       "call handle_trap\n"

                       "lw ra,  4 * 0(sp)\n"
                       "lw gp,  4 * 1(sp)\n"
                       "lw tp,  4 * 2(sp)\n"
                       "lw t0,  4 * 3(sp)\n"
                       "lw t1,  4 * 4(sp)\n"
                       "lw t2,  4 * 5(sp)\n"
                       "lw t3,  4 * 6(sp)\n"
                       "lw t4,  4 * 7(sp)\n"
                       "lw t5,  4 * 8(sp)\n"
                       "lw t6,  4 * 9(sp)\n"
                       "lw a0,  4 * 10(sp)\n"
                       "lw a1,  4 * 11(sp)\n"
                       "lw a2,  4 * 12(sp)\n"
                       "lw a3,  4 * 13(sp)\n"
                       "lw a4,  4 * 14(sp)\n"
                       "lw a5,  4 * 15(sp)\n"
                       "lw a6,  4 * 16(sp)\n"
                       "lw a7,  4 * 17(sp)\n"
                       "lw s0,  4 * 18(sp)\n"
                       "lw s1,  4 * 19(sp)\n"
                       "lw s2,  4 * 20(sp)\n"
                       "lw s3,  4 * 21(sp)\n"
                       "lw s4,  4 * 22(sp)\n"
                       "lw s5,  4 * 23(sp)\n"
                       "lw s6,  4 * 24(sp)\n"
                       "lw s7,  4 * 25(sp)\n"
                       "lw s8,  4 * 26(sp)\n"
                       "lw s9,  4 * 27(sp)\n"
                       "lw s10, 4 * 28(sp)\n"
                       "lw s11, 4 * 29(sp)\n"
                       "lw sp,  4 * 30(sp)\n"
                       "sret\n");
}

/*
カーネルエントリ直前のspをsscratchレジスタに退避させておき（csrw)、
その時のレジスタの他の部分を全てメモリにコピーし（sw,コピー先はspからのoffsetで計算)
sscratchレジスタにあるspもスタック領域にコピーし（csrr a0, sscratch;のように
一時レジスタa0経由で）
その後 call handle_trap で例外処理を呼び出すとレジスタは刷新され、
例外処理が終わったら、lwで先ほどのスタック領域の値をレジスタに再度格納し、
最後に、sretで割り込み前の処理を再開
*/

/*
１、起動直後の sp は OS 用スタック領域の先頭（まだ何も保存されていない）。
２、例外ハンドラは呼ばれた瞬間、どこに作業用スタックを置くか知らない。
３、sscratchレジスタに起動直後のspを保存することで、ハンドラが安全にスタックを使える。

↑の本質：
kernel_entry は Supervisor モード（S）で動くことを想定しています。
Supervisor モードの例外（トラップ）では、CPU は
自動的にスタックフレームを作らず、
Sモード用のスタックを自分で準備する必要があります。
だから sscratchなどの「特権レベル専用レジスタ」が必要になります。

起動時の安全なカーネルスタックの先頭アドレスを sscratch
に退避しておき、例外ハンドラ内で参照できるようにしています。
一方、kernel_main内では、↓

kernel_entry で既に sp をカーネル用スタックの安全な領域にセット しているため、C
言語の関数呼び出しは普通に動きます。
C コンパイラは関数呼び出し時に sp を基準にスタックフレームを作る
ので、明示的にどこにスタックを置くかを教える必要はありません。
つまり、kernel_main 内では「現在の sp
が有効なスタック領域を指している」という前提で、普通に push/pop
やローカル変数確保が可能 です。
*/

/*
カーネルエントリがやること
１、CPU状態（レジスタのsp）を保存
    csrw sscratch, sp;
２、どの種類のトラップか識別
３、適切なハンドラを呼び出す（handle_trap）
４、終わったらレジスタを復元
*/

/*
csrw（Control and Status Register Write）：
CPUの制御用/特権レジスタに値を書き込む命令

csrw <書き込み先(制御用レジスタ)> <値を持つ汎用レジスタ>;
*/

/*
sw ra, 4 * 0(sp)

sw … Store Word（レジスタの値をメモリに書き込む）
ra … return address レジスタ。関数から戻るときのアドレスが入っています。
sp … スタックポインタ（Stack Pointer）
4 * 0(sp) … スタック上のアドレスを意味しています（offset + base形式）
*/

/*
sret: RISC-Vにおいて、特権モードからユーザーモードに戻る命令
*/

/*
┌───────────────┐
│  ユーザモード実行中 │
└───────┬───────┘
          │ トラップ発生
          ▼
┌────────────────────────────┐
│ kernel_entry:                 │
│   csrw sscratch, sp           │ ← (1) 旧SP保存
│   addi sp, sp, -4*31          │ ← (2) 退避領域確保
│   sw ...                      │ ← (3) レジスタ退避
│   mv a0, sp                   │
│   call handle_trap            │ ← (4) Cハンドラ呼出
│   lw ...                      │ ← (5) レジスタ復元
│   sret                        │ ← トラップ復帰
└────────────────────────────┘
          │
          ▼
┌───────────────┐
│ 元の命令に戻る │
└───────────────┘

*/

/*
CPUハードウェア と ISA仕様書 が先に存在している
→ ここでレジスタの名前や命令の動作が定義済み
*/