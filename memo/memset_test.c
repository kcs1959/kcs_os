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
┌───────────────────────┐
│ 関数: boot()           │
│ - sp = x2 (0x8022004c)│
│ - ra = x1 (戻り先)    │
│ - a0/a1/a2 = memset引数│
└───────────────────────┘
*/