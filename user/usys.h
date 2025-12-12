#ifndef USER_H
#define USER_H
// システムコールを扱うヘッダ

#include "common.h"

__attribute__((noreturn)) void exit(int status);
void putchar(char ch);
int getchar(void);
int syscall(int sysno, int arg0, int arg1, int arg2);
int create_file(const char *name, const uint8_t *data, uint32_t size);
void sys_list_root_dir(void);
void sys_concat_first_file(void);
int printf(const char *fmt, ...);
void sys_shutdown(void);


#endif
