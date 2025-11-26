/*
Q. Userモード、Supervisorモードで操作する、とは
具体的に何をすることなのか

ヒント：syscall()→ecall→handle_trap()→handle_syscall()→sret

A. stvecレジスタにkernel_entryのアドレスが入っていて、
適宜そのレジスタの値を読み出す
そのとき、(ecallの場所)+4のアドレスをsepcレジスタに入れる
こと。
*/

struct sbiret {
  long error;
  long value;
};

struct sbiret sbi_call(long arg0, long arg1, long arg2, long arg3, long arg4,
                       long arg5, long fid, long eid) {
  register long a0 __asm__("a0") = arg0;
  register long a1 __asm__("a1") = arg1;
  register long a2 __asm__("a2") = arg2;
  register long a3 __asm__("a3") = arg3;
  register long a4 __asm__("a4") = arg4;
  register long a5 __asm__("a5") = arg5;
  register long a6 __asm__("a6") = fid;
  register long a7 __asm__("a7") = eid;

  __asm__ __volatile__("ecall"
                       : "=r"(a0), "=r"(a1)
                       : "r"(a0), "r"(a1), "r"(a2), "r"(a3), "r"(a4), "r"(a5),
                         "r"(a6), "r"(a7)
                       : "memory");
  return (struct sbiret){.error = a0, .value = a1};
}

/*
CPU に複数の特権レベルがあります：
・User モード：普通のアプリが動く
・Supervisor モード：OSカーネルが動く
・Machine モード：最も高権限、ブートやハードウェア制御用

「SBIは、OS（Supervisor）からハードウェア（Machineモード）に対するサービス呼び出しの仕組み」

ecall命令（アセンブリ）は
現在の特権モードから上位モードへの環境呼び出しに使われます。
*/

/*
sbiret: SBI呼び出しの戻り値 を格納する構造体（クラスみたいな）
sbi_call関数の戻り値は、struct sbiret と表される
*/

/*
RISC-Vでは、SBI呼び出しのパラメータは レジスタ a0〜a7 に入れる必要があります。
「iは頻繁に使われる変数だから、メモリではなくCPUレジスタに置いてほしい」
というコンパイラへのお願い
・メモリ上にないため、&でアドレスを取れない

低級言語であるCは、「ストレージクラス指定子」を持つ
・auto（スタック）← int x = 5; は、auto int x = 5; と同じ
・register（レジスタ）← 高速アクセス。アドレス取得不可
・static　← static int x = 5; は関数外でも使える（DATAセクション）
・extern ← 他ファイルで宣言された変数の利用時
・typedef ← 既存のデータ型にエイリアスをつける（変数は作らない）
*/

/*
volatile: コンパイラの最適化を阻止
*/
volatile int flag = 0;

void interrupt_handler() {
  flag = 1; // 割り込みで更新される
}

void main_loop() {
  while (!flag) {
    // flagが変わるのを待つ
  }
}
/*
において、flag が volatile でない場合、
flagはmain_loop内で書き換えられないため、コンパイラは「flagが常に0だ」と最適化する
→ while (!flag) {} は無限ループになってしまう

volatileが付いていない場合、
コンパイラは「ループ中に値は変わらない」と判断して、変数をレジスタに置いてしまうことがあります。
flag を volatileにすることで、
コンパイラは、毎回必ずメモリのデータ領域（∵グローバル変数）からflagを読み直す

（↑割り込みハンドラは、通常のCPUレジスタにコピーされた変数の値は直接変えられない）
*/