#include <stdbool.h>
#include <stdio.h>

#define PANIC(fmt, ...)                                                        \
  do {                                                                         \
    printf("PANIC: %s:%d: " fmt "\n", __FILE__, __LINE__, ##__VA_ARGS__);      \
    while (1) {                                                                \
    }                                                                          \
  } while (0)

/*
カーネルパニック：エラーログを出して無限ループに入る

「複数文のマクロ → do-while ループで囲む！」
「do { ... } while(0) は マクロの中で複数文を安全にまとめるテクニック」

マクロの中で printf と無限ループの while(1) の2つの文があるので、普通に { ... }
で囲っただけだと、マクロを使ったときに構文上の問題が出る場合があります。
*/

int main(void) {
  bool error = false;

  if (error)
    PANIC("something went wrong");
  else
    printf("no error\n");
}

/*
もしwhile{0}がなく、
#define PANIC(*fmt, ...) printf(fmt); while(1){}
だけだった場合、↑のPANICをマクロ展開すると

if (error)
    printf("PANIC: %s:%d: error occurred\n", __FILE__, __LINE__);
    while (1) {
    };
else
    printf("no error\n");

中括弧なしのif文の後に続くのは1文である必要があるが、
PANICマクロは printf, while(1) の2文に展開されるため、
コンパイルエラー「elseがどのifに対応しているかわからない」


do { ... } while(0); → 全体が1つの文として扱われるので 構文が正しい。
中括弧なしのifの後に複数文がマクロ展開されても大丈夫！！
*/