#include "../../common.h"
#include "./sbi_call_test.c"

long getchar(void) {
  struct sbiret ret = sbi_call(0, 0, 0, 0, 0, 0, 0, 2);
  return ret.error;
}

/*
今回はデバッグコンソールなので、
ret構造体のvalueではなくerror属性を返す
*/

/*
getcharで入力を受け付け、入力がhelloだったらputcharで文章を表示、
違ったら入力値をそのまま返す
というhelloコマンドを実装する。
*/
void main(void) {
  while (1) {
  prompt:
    printf("> ");
    char cmdline[128];
    for (int i = 0;; i++) {
      char ch = getchar();
      putchar(ch);
      if (i == sizeof(cmdline) - 1) {
        printf("command line too long\n");
        goto prompt;
      } else if (ch == '\r') {
        printf("\n");
        cmdline[i] = '\0';
        break;
      } else {
        cmdline[i] = ch;
      }
    }

    if (strcmp(cmdline, "hello") == 0)
      printf("Hello world from shell!\n");
    else
      printf("unknown command: %s\n", cmdline);
  }
}