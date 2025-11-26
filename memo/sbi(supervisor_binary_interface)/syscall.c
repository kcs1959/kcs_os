/*
基本ルール：
ユーザーはハードウェアを直接叩かず、カーネル経由で操作する。

SBI = カーネルがハードウェアにアクセスするためのRISC-V固有のインターフェイス
ls / cat / echo = 単なるユーザ空間アプリケーション
間に システムコール が入るので、ユーザーがSBIを直接呼んでいるわけではない
*/

// これはユーザーが直接sbiを叩いてて良くない
void putchar(char ch) { sbi_call(ch, 0, 0, 0, 0, 0, 0, 1); }

// まずsyscall。これは標準sbi.h内で定義されている
int syscall(int sysno, int arg0, int arg1, int arg2) {
  register int a0 __asm__("a0") = arg0;
  register int a1 __asm__("a1") = arg1;
  register int a2 __asm__("a2") = arg2;
  register int a3 __asm__("a3") = sysno;

  __asm__ __volatile__("ecall"
                       : "=r"(a0)
                       : "r"(a0), "r"(a1), "r"(a2), "r"(a3)
                       : "memory");

  return a0;
}

// 引数sysnoは、
#define SYS_PUTCHAR 1
// と定義したので、
void putchar(char ch) { syscall(SYS_PUTCHAR, ch, 0, 0); }
// のように使える。

/*
システムコール番号とは・・・
カーネルが提供する機能（＝システムコール）を整数の番号で指定する
０：exit（プロセス終了）
１：write（ファイル書き込み）
２：read（ファイル読み取り）　など

register int xxx __asm__("ax") = yyy;
「xxxというCの変数を作り、それをレジスタaxに置く。ただし初期値はyyy」

その後のインラインアセンブリ
__asm__ __volatile__("ecall"
                        : "=r"(a0)
                        : "r"(a0), "r"(a1), "r"(a2), "r"(a3)
                        : "memory");
では、register int... の逆を行っている
＝ecallでsupervisorモードにした後、カーネルから返ってきたレジスタの値をC変数にする
*/

/*
[ユーザプログラム]
    |
    | ecall（呼び鈴）
    v
[CPU（ハード）]  ← 自動の低レイヤー
    - U→Sモードへ遷移
    - scause,sepc設定
    - stvecへジャンプ
    |
    v
[OSのtrap handler] ← あなたのコード
    - syscall番号読み取り
    - sys_write呼ぶ
    - UARTへ書き込み
    - a0に戻り値セット
    |
    v
[sretでユーザに帰還]
*/

#include "../../kernel.h"
void handle_trap(struct trap_frame *f) {
  uint32_t scause = READ_CSR(scause);
  uint32_t stval = READ_CSR(stval);
  uint32_t user_pc = READ_CSR(sepc);
  if (scause == SCAUSE_ECALL) {
    handle_syscall(f);
    user_pc += 4;
  } else {
    PANIC("unexpected trap scause=%x, stval=%x, sepc=%x\n", scause, stval,
          user_pc);
  }

  WRITE_CSR(sepc, user_pc);
}

/*
handle_trap関数内で実際のシステムコールの処理をいろいろ定義する。

ユーザーがecallを実行すると、
CPUが自動でhandle_trapを呼ぶ（具体的には↓）

１、ユーザモード → Supervisor モードへ遷移（権限レベルの切り替え）

２、レジスタ設定：
sepc ← 呼び出し元のプログラムカウンタ（復帰用）
scause ← 例外／割込みの種類を格納
※

３、制御を trap ベクタに移す：
stvec レジスタに書かれたアドレスへジャンプ

以上はCPUの仕様。この仕様を満たすように、カーネルでは、↓

kernel_main関数の最初で、
WRITE_CSR(stvec, (uint32_t)kernel_entry);
stvecレジスタにkernel_entryのアドレスを入れる。
kernel_entry内で、call handle_trapによりsyscallが呼び出される。

まとめ
stvecレジスタを参照するのは仕様。
stvecレジスタにkernel_entryのアドレスを入れておくのはOS。
*/

/*
sepcレジスタは、プログラムカウンタを保持するレジスタ。
その仕様を満たすように、
uint32_t user_pc = READ_CSR(sepc);
と定義している。
*/

/*
ecallを呼んだら、cpuが自動的にstvecレジスタの値を読み、
kernel_entryに飛ぶ。
そこで、handle_trapが呼び出される。
ここでは、user_pcにsepcレジスタの値を入れたのち、
それに4を足したものを再度sepcレジスタに格納する。
その後、sretでsepcの値からユーザープログラムが再開される。
*/