#include "user.h"

void main(void) {
  __asm__ __volatile__("unimp");
  for (;;)
    ;
}