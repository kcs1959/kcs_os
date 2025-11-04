__attribute__((section(".text.boot"))) __attribute__((naked)) void boot(void) {
  __asm__ __volatile__("mv sp, %[stack_top]\n"
                       "j kernel_main\n"
                       :
                       : [stack_top] "r"(__stack_top));
}

/*
ブート：スタック初期化＋カーネルへのジャンプ
ROM 先頭や初期化済みメモリに置かれ、CPU 起動後すぐに呼ばれる
最初のエントリポイント

CPU が起動すると、レジスタは未定義状態で、スタックもセットされていません。
スタックポインタ (sp) を正しいメモリ領域に設定 する必要があります。
その後にメモリ初期化（BSSクリアなど）や、Cの初期化ルーチンを呼び出す流れになります。
*/

/*
naked: Cの通常の関数規約を無視
→コンパイラはその関数のprolog, epilogを省略する
CPU 起動直後や割り込みハンドラなど、
スタックやレジスタが未初期化の状態で動く必要があるコードに使用する。


・具体例
void normal(int i, int j) {
    i = i + j;
}
のアセンブリを考える。↓


_normal	PROC

## ここがprologコード
; 1    : void normal(int i, int j) {

        push	ebp
        mov	ebp, esp

## ここから関数本体
; 2    : 	i = i + j;

        mov	eax, DWORD PTR _i$[ebp]
        add	eax, DWORD PTR _j$[ebp]
        mov	DWORD PTR _i$[ebp], eax

## ここがepilogコード
; 3    : }

        pop	ebp
        ret	0
_normal	ENDP
_TEXT	ENDS


上のように、普通は prolog, epilog の部分が存在する（下記参照）
naked属性を持つ関数は、それに相当するアセンブラが作られない。
*/

/*
prolog:
push        ebp                ; Save ebp
mov         ebp, esp           ; Set stack frame pointer
sub         esp, localbytes    ; Allocate space for locals
push        <registers>        ; Save registers

epilog:
pop         <registers>   ; Restore registers
mov         esp, ebp      ; Restore stack pointer
pop         ebp           ; Restore ebp
ret                       ; Return from function


prologでやってること

新たに関数を実行するときは、その前に実行されていた関数の始まるアドレスを表したebpレジスタの中身を、
一旦スタック領域のどこかに保存する

１、push ebp
では、次の操作をまとめて行なっている

esp = esp - 4; ←レジスタが全て4バイト(32ビット)幅
*(uint32_t*)esp = ebp;

スタックポインタを下げ（4バイト分の空き領域を確保）し、
前の関数のebpの値をそこに置く

EBPやESPはメモリ上の「場所（アドレス）」を覚えているレジスタ
実際のデータ（関数の引数や変数）はメモリ上に置かれています。
スタックに保存するのは、アドレスそのものではなく、CPUレジスタに入っている32ビットの数値

２、mov ebp, esp;
ebpレジスタの中身を、esp（スタック先頭）とする

３、sub esp, localbytes;
そこからespを下に動かしてローカル変数分の領域を確保
ローカル変数は、ebp - offset で計算する
*/

/*
メモリ空間（RAM上）
──────────────────────────
0x7ffef020  ← [EBP]  ← この関数の基準点
   ↑
   │
0x7ffef000  ← [ESP]  ← 現在のスタックの先頭
──────────────────────────

CPU内部
──────────────────────────
  EBPレジスタ = 0x7ffef020
  ESPレジスタ = 0x7ffef000
──────────────────────────
*/