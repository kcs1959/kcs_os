/*
実際は、たくさんの関数を呼び出す→切り替えが必要
必要に応じて、複数実行したい関数を各々プロセスとし、
コンテキストスイッチで、実行する関数を切り替えることができる

create_processでプロセス用のスタック領域を確保
（＝callee-savedレジスタの状態を入れるとこ）

procA↔︎procBのプロセス切り替え
CPU の sp は「今実行中のプロセス用スタックのトップ」を指すレジスタ
これはプロセスごとに異なるスタック領域を持つため、
切り替え時に保存・復元する必要がある
processAを中断する時は、
switch_context で sw sp, &proc_a->sp とすることで、
CPU sp を proc_a 用のスタックトップとして保存できる（今の状態を退避）
processAに戻るときは、
lw sp, &proc_a->sp で 中断時のスタックトップ を CPU sp に復元
lw ra, s0…s11 で前回退避したレジスタを復元する
*/

#include "../../common.h"
#include "../../kernel.h"
#include <stdint.h>

typedef uint32_t vaddr_t;

#define PROCS_MAX 8     // 最大プロセス数
#define PROC_UNUSED 0   // 未使用のプロセス管理構造体
#define PROC_RUNNABLE 1 // 実行可能なプロセス

struct process {
  int pid;             // プロセスID
  int state;           // プロセスの状態: PROC_UNUSED または PROC_RUNNABLE
  vaddr_t sp;          // コンテキストスイッチ時のスタックポインタ
  uint8_t stack[8192]; // カーネルスタック
};

struct process procs[PROCS_MAX];

// プロセスを作る
struct process *create_process(uint32_t pc) {
  // 空いているプロセス管理構造体を探す
  struct process *proc = NULL;
  int i;
  for (i = 0; i < PROCS_MAX; i++) {
    if (procs[i].state == PROC_UNUSED) {
      proc = &procs[i];
      break;
    }
  }

  if (!proc)
    PANIC("no free process slots");

  // switch_context() で復帰できるように、スタックに呼び出し先保存レジスタを積む
  uint32_t *sp = (uint32_t *)&proc->stack[sizeof(proc->stack)];
  *--sp = 0;            // s11
  *--sp = 0;            // s10
  *--sp = 0;            // s9
  *--sp = 0;            // s8
  *--sp = 0;            // s7
  *--sp = 0;            // s6
  *--sp = 0;            // s5
  *--sp = 0;            // s4
  *--sp = 0;            // s3
  *--sp = 0;            // s2
  *--sp = 0;            // s1
  *--sp = 0;            // s0
  *--sp = (uint32_t)pc; // ra

  // 各フィールドを初期化
  proc->pid = i + 1;
  proc->state = PROC_RUNNABLE;
  proc->sp = (uint32_t)sp;
  return proc;
}

// コンテキストスイッチ
__attribute__((naked)) void switch_context(uint32_t *prev_sp,
                                           uint32_t *next_sp) {
  __asm__ __volatile__(
      // 実行中プロセスのスタックへレジスタを保存
      "addi sp, sp, -13 * 4\n"
      "sw ra,  0  * 4(sp)\n"
      "sw s0,  1  * 4(sp)\n"
      "sw s1,  2  * 4(sp)\n"
      "sw s2,  3  * 4(sp)\n"
      "sw s3,  4  * 4(sp)\n"
      "sw s4,  5  * 4(sp)\n"
      "sw s5,  6  * 4(sp)\n"
      "sw s6,  7  * 4(sp)\n"
      "sw s7,  8  * 4(sp)\n"
      "sw s8,  9  * 4(sp)\n"
      "sw s9,  10 * 4(sp)\n"
      "sw s10, 11 * 4(sp)\n"
      "sw s11, 12 * 4(sp)\n"

      // スタックポインタの切り替え
      "sw sp, (a0)\n" // 旧プロセスのspを保存（退避）
      "lw sp, (a1)\n" // 新プロセスのspをロード

      // 次のプロセスのスタックからレジスタを復元
      "lw ra,  0  * 4(sp)\n"
      "lw s0,  1  * 4(sp)\n"
      "lw s1,  2  * 4(sp)\n"
      "lw s2,  3  * 4(sp)\n"
      "lw s3,  4  * 4(sp)\n"
      "lw s4,  5  * 4(sp)\n"
      "lw s5,  6  * 4(sp)\n"
      "lw s6,  7  * 4(sp)\n"
      "lw s7,  8  * 4(sp)\n"
      "lw s8,  9  * 4(sp)\n"
      "lw s9,  10 * 4(sp)\n"
      "lw s10, 11 * 4(sp)\n"
      "lw s11, 12 * 4(sp)\n"
      "addi sp, sp, 13 * 4\n"
      "ret\n");
}

/*
プロセスを作るcreate_process()
１、プログラムカウンタ＝実行開始アドレス　を引数として渡す
２、プロセステーブルprocs[]から空きスロットを探し、
    proc = &procs[i];で確保
３、後々callee-savedレジスタ（s0~s11,ra)をスタックに退避するため、
    先ほどのproc（空き）をその分の領域として、0で初期化
    raレジスタ用の領域は、対象プロセスの最初の命令アドレス(pc)としておく
４、pid, state, sp を設定し、それらをまとめたproc（構造体）を返す
    例えばstateはPROC_RUNNABLEに設定
*/

/*
*--sp は、
１、まずスタックポインタ sp を 4 バイト下げる（--sp）
２、その場所に値を書き込む（*sp = 値）

これにより、メモリのスタックトップに復帰すべきreturn adress,
その下にs0-s11が並ぶ

*/

/*
下方向に伸びるスタックの初期化では、
proc構造体で定義したstack配列の末尾のアドレスをスタックポインタとする
↑　&proc->stack[sizeof(proc->stack)]
*/

/*
context_switchの本質は、
sw sp, prev_sp
lw sp, next_sp

sw sp, prev_spで、
レジスタのspという領域（ここには現在実行中の命令アドレスが格納されている）
を、メモリのprev_spで表された領域に読み出す
この次に、lw sp, next_spで、
レジスタのspという領域を今度は切り替え先の命令アドレスにする
↓
この時、CPUのspは、切り替え先プロセスのスタックトップを表す
ここから、lw で、
切り替え先プロセスのスタックに保存されている値（CPU状態）をレジスタに復元する

・もし切り替え先が初めて実行されるなら、
create_processで積んだ0とpcが置かれている
・すでに実行済みで中断したなら、
switch_context前半で退避した値がスタックに入っているので
それを復元し、中断したやつを再開できる
*/

/*
caller-saved: ra, t0–t6, a0–a7   ← 呼び出す側でスタックに退避
callee-saved: sp, s0–s11         ← 呼び出された側が退避

複数の関数がレジスタの値を好き勝手使ったら、値が消えてしまうかも
→呼び出す/呼び出される側のどちら側がレジスタの値を守る責任を持つか

・callee-savedは、呼び出された側で退避してくれるので、
呼び出す側は安心してその値を使える
void A() {
    int x = 10;   // s0 に入っている
    B();          // B が終わったあとも s0 は壊されない前提
    printf("%d", x); // ちゃんと10が残っている
}
→callee-savedは、他関数を呼び出しても壊されない前提で使える

・caller-savedは、呼び出し後にもその値を使いたい場合は、
呼び出す側で退避しておく必要がある
void A() {
    int y = 42;   // t0 に入っている
    // B を呼ぶと t0 が上書きされるかも！
    // だから呼ぶ前に保存する
    push(t0);
    B();
    pop(t0);      // 戻ってきたら復元
}
→caller-savedは、他関数呼び出しによって壊れる可能性がある

・switch_contextを呼び出す際に、
callee-savedレジスタに実行中関数の命令のアドレスを格納できるように、
Create_processの時点でこれらのレジスタを0で初期化しておく。
使い方としては、↓
*/

void delay(void) {
  for (int i = 0; i < 30000000; i++)
    __asm__ __volatile__("nop"); // 何もしない命令
}

struct process *proc_a;
struct process *proc_b;

void function_a(void) {
  printf("starting process A\n");
  while (1) {
    putchar('A');
    switch_context(&proc_a->sp, &proc_b->sp);
    delay();
  }
}

void function_b(void) {
  printf("starting process B\n");
  while (1) {
    putchar('B');
    switch_context(&proc_b->sp, &proc_a->sp);
    delay();
  }
}

void kernel_main(void) {
  proc_a = create_process((uint32_t)function_a);
  proc_b = create_process((uint32_t)function_b);
  function_a();
}

/*
ページテーブルを導入したので・・・
→同じ仮想アドレスでも違う物理アドレスにアクセスできる

・対象プロセスのスタック領域確保
・procs配列を返す
以外に、
・対象プロセスのページマッピング
が必要。

※プロセスのスタック領域を確保するだけでなく、
そのプロセスの物理アドレス（今回はその中身の値はテーブルのアドレスと一致）
を格納したページテーブル用の領域も【物理】メモリ上に確保する必要がある

※「プロセスのスタック領域」とは、プロセスが途中で中断されたとき、
その時のcpuレジスタをメモリに退避させる用のスタック領域
最初は0で初期化しておく。

※仮想メモリ上には、・プロセスのコード領域・各プロセスのスタック領域がある。
物理メモリ上には、
・プロセスのコード領域/スタック領域
・各プロセスのページテーブル
・カーネル
が存在する。

*/
void *create_process_extracted(const void *image, size_t image_size) {
  for (paddr_t paddr = (paddr_t)__kernel_base; paddr < (paddr_t)__free_ram_end;
       paddr += PAGE_SIZE)
    map_page(page_table, paddr, paddr, PAGE_R | PAGE_W | PAGE_X);

  for (uint32_t off = 0; off < image_size; off += PAGE_SIZE) {
    paddr_t page = alloc_pages(1);

    size_t remaining = image_size - off;
    size_t copy_size = PAGE_SIZE <= remaining ? PAGE_SIZE : remaining;

    // 確保したページにバイナリイメージをコピー
    memcpy((void *)page, image + off, copy_size);

    // ページテーブルマッピング（物理メモリ上でページテーブル領域を確保）
    map_page(page_table, USER_BASE + off, page,
             PAGE_U | PAGE_R | PAGE_W | PAGE_X);
  }
}

/*
プロセスを、ユーザーモードで動かしたいので・・・

アプリケーションのバイナリイメージをユーザー空間にコピーして独立させる
｜｜
引数を、実行関数の開始アドレス＝プログラムカウンタにするのではなく、
*image, image_size とする。

・ユーザー空間は、user.ldで . = 0x1000000; と定義した。

・*image: プロセスにロードするイメージ（コンパイル済みのバイナリ）の先頭アドレス
image_size: そのバイナリのコードのサイズ
cpuの戻りアドレス（raレジスタ）には、user_entryを入れる

・同じバイナリ（イメージ）を複数プロセスにロードしたい時、
バイナリをユーザー空間にコピーせずに、そのままpcを渡すと、
複数プロセスが同じ物理ページを参照し、
一方の書き換えで他方が変わってしまったりする
↑プロセス分離の原則に反する
*/