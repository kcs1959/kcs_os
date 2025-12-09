#ifndef COMMON_H
#define COMMON_H
// TODO: このファイルの説明を簡潔な日本語で書く

#include "../common_types.h"

int printf(const char *fmt, ...);
void putchar(char ch);

void *memset(void *buf, int c, size_t n);
void *memcpy(void *dst, const void *src, size_t n);
char *strcpy(char *dst, const char *src);
int strcmp(const char *s1, const char *s2);
int strncmp(const char *s1, const char *s2, uint32_t n);
int rand(void);
void srand(unsigned int seed);

#endif
