// printf は OSカーネルの「目に見えるデバッグインターフェース」
// として使われます。

#pragma once
// は、このヘッダファイルが何回 #include されても、1回だけ展開されるようにする

// 可変長引数マクロの定義
#define va_list __builtin_va_list
#define va_start __builtin_va_start
#define va_end __builtin_va_end
#define va_arg __builtin_va_arg
// 普通は <stdarg.h> を #include することで va_list が使えるようになる
// #define 名前 値 はコンパイル前の置換ルール ←エイリアス？
#define PI 3.14159;
#define SQUARE(x) ((x) * (x));
// のように、引数を取るマクロもある

// 関数のプロトタイプ宣言もヘッダファイル内で行う
void printf(const char *fmt, ...);