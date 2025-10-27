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