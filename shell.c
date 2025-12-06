#include "user.h"

int main() {
  while (1) {
  prompt:
    printf("\x1b[33m> \x1b[39m");
    char cmdline[128];
    int i = 0;
    while (true) {
      char ch = getchar();
      if ((ch == '\x7f') && (cmdline[0] != '\0')) {
        putchar('\b');
        putchar(' ');
        putchar('\b');
        i--;
        cmdline[i] = '\0';
        continue;
      }
      if (ch == '\x03') {
        printf("\x1b[30;47m^C\x1b[39;49m\n");
        goto prompt;
      }
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
      i++;
    }
    if (strcmp(cmdline, "hello") == 0)
      printf("\x1b[36mHello world from shell!\n\x1b[39m");
    else if (strcmp(cmdline, "exit") == 0)
      exit();
    else if (strcmp(cmdline, "ls") == 0)
      list_root_dir();
    else if (strcmp(cmdline, "cat") == 0)
      concatenate();
    else if (strcmp(cmdline, "ohgiri") == 0) {
      int r = rand() % 3;
      if (r == 0)
        printf(
            "パクツイする人、ネタツイーター(ネタツイを食い物にしているので)\n");
      else if (r == 1)
        printf("熱海での自分探しの旅の末、ついに自分が見つかりました "
               "(あったme)\n");
      else if (r == 2)
        printf("このモデル、ついに2種類のカタカナを見分けられるようになりました"
               "!(キかイ学習)\n");
    } else
      printf("\x1b[31mUnknown command: %s\n\x1b[39m", cmdline);
  }
}
