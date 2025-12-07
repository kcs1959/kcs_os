#ifndef USER_H
#define USER_H

#include "lib/common.h"

__attribute__((noreturn)) void exit(int status);
void putchar(char ch);
int getchar(void);
int create_file(const char *name, const uint8_t *data, uint32_t size);
void sys_list_root_dir(void);
void sys_concat_first_file(void);

#endif
