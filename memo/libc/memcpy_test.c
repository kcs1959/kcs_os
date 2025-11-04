#include <stdio.h>
typedef unsigned char uint8_t;

void *memcpy(void *dst, const void *src, size_t n) {
  uint8_t *d = (uint8_t *)dst;
  const uint8_t *s = (const uint8_t *)src;
  while (n--) {
    *d++ = *s++;
  }
  return dst;
}

// 標準のmemcpyを使えないのは、自作OS上でlibcが存在しないから
/*
例えば printf, memcpy, malloc, strlen などはすべて libc に入っています。
普段 #include <string.h> や #include <stdio.h> と書くと、実は libc
にある関数の宣言や実体 を呼び出しています。
libc がないので、標準の memcpy は使えません。
そこで OS 開発者は、自作の memcpy や memset
を作って、カーネルやブート時に必要なメモリ操作 を行います。
*/

// srcからnバイト分だけdstに移動する関数
// return dst; でコピー先の先頭アドレスを返す
// 1バイト単位でコピーするために、void型のdst,srcを uint8_t * にキャスト変換
// srcは変更不可なので const で宣言
// 汎用関数の引数を void * にすることで型を問わず受け取れる
// ↑コピー対象は整数でも文字列でも構造体でもなんでも良い
