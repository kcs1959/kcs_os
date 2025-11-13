/*
関数の引数の数を動的に処理したい場合（printfなど）
可変長の引数を持つ関数

va_starts(vargs, fmt);　で引数の読み取りを開始
va_arg(vargs, 型)　で次の引数を読み取り
va_ends(vargs); で読み取り終了

ポイント：
・今読んでいる引数は vargs で表す
これを、最初に va_list vargs; で定義する

・va_starts(vargs, fmt);
は、fmt の次の引数から可変長引数の読み取り開始
fmt は "user:%s" みたいなフォーマット文字列

・printf → 引数を直接渡す
vprintf → va_list を渡して間接的に可変長引数を渡す
ラッパー関数や可変長引数を中継するときには必ず vprintf 系を使います。

・自作printfでは、
while文でfmtを1つずつずらしていき、%が現れたらその次の文字に注目
%の次の文字が
・\0 （文字列の終端）なら、%をそのまま出力して va_end に飛ぶ
・% （%%のケース）なら、%をそのまま出力して次の文字(break)
・s, d, x ならフォーマット指定子の処理
*/

#include <stdarg.h> // va_list
#include <stdio.h>

void log_message(const char *fmt, ...) {
  va_list args;
  va_start(args, fmt);

  printf("[LOG] ");
  vprintf(fmt, args);
  printf("\n");

  va_end(args);
}

int main(void) {
  int value = 42;
  const char *user = "Alice";

  log_message("Program Started");
  log_message("User: %s, Value: %d", user, value);
  log_message("Computation result: %.2f", 3.14159);
}