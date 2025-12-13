#ifndef COMMON_H
#define COMMON_H
// ユーザ空間の入出力・標準関数の宣言をまとめたヘッダー

#include "common_types.h"

int printf(const char *fmt, ...);
int vprintf(putc_fn_t putc, const char *fmt, va_list vargs);
void putchar(char ch);

void *memset(void *buf, int c, size_t n);
void *memcpy(void *dst, const void *src, size_t n);
char *strcpy(char *dst, const char *src);
int strcmp(const char *s1, const char *s2);
int strncmp(const char *s1, const char *s2, uint32_t n);
int rand(void);
void srand(unsigned int seed);

#endif
