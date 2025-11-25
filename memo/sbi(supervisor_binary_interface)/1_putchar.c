#include "../../common.h"
#include "./sbi_call_test.c"

void putchar(char ch) { sbi_call(ch, 0, 0, 0, 0, 0, 0, 1); }

// 以下は、ユーザーアプリケーションでの使用例
void main(void) { printf("Hello World from shell!\n"); }