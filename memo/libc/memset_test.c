#include <stdio.h>

typedef unsigned char uint8_t;
typedef unsigned int uint32_t;

void *memset(void *buf, char c, int n) {
  uint8_t *p = (uint8_t *)buf;
  while (n--)
    *p++ = c;
  return buf;
}

int main() {
  char str[10];
  memset(str, 'A', 5);
  str[5] = '\0';
  printf("%s\n", str);
  return 0;
}

/*
void *memset(void *buf, char c, size_t n)
void *buf: 書き込み対象のメモリ領域
char c: 埋めたい値（1バイト）
size_t n: 書き込むバイト数


使用例：
char data[5];
memeset(data, 'A', 5);

int arr[4];
memset(arr, 0, sizeof(arr));
→配列全体をゼロ初期化

*/