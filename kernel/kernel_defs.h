#ifndef KERNEL_DEFS_H
#define KERNEL_DEFS_H
// カーネル共通の定義と基本ランタイム宣言を提供するヘッダー

#include "common_types.h"

#define align_up(value, align) __builtin_align_up(value, align)
#define is_aligned(value, align) __builtin_is_aligned(value, align)
#define offsetof(type, member) __builtin_offsetof(type, member)

// カーネルI/O
void kputchar(char ch);
long kgetchar(void);
int printf(const char *fmt, ...);
int vprintf(const char *fmt, va_list vargs);

// カーネル側で使う最小限のランタイム
void *memset(void *buf, int c, size_t n);
void *memcpy(void *dst, const void *src, size_t n);
char *strcpy(char *dst, const char *src);
int strcmp(const char *s1, const char *s2);
int strncmp(const char *s1, const char *s2, uint32_t n);
int rand(void);
void srand(unsigned int seed);

#endif
