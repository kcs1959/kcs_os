#ifndef COMMON_TYPES_H
#define COMMON_TYPES_H
// TODO: このファイルの説明を簡潔な日本語で書く

typedef unsigned char uint8_t;
typedef unsigned short uint16_t;
typedef unsigned int uint32_t;
typedef unsigned long long uint64_t;
typedef uint32_t size_t;
typedef uint32_t paddr_t;
typedef uint32_t vaddr_t;
typedef uint8_t bool;

#define true 1
#define false 0
#define NULL ((void *)0)

#define va_list __builtin_va_list
#define va_start __builtin_va_start
#define va_end __builtin_va_end
#define va_arg __builtin_va_arg

#define PAGE_SIZE 4096

#define SYS_PUTCHAR 1
#define SYS_GETCHAR 2
#define SYS_EXIT 3
#define SYS_CREATE_FILE 4
#define SYS_LIST_ROOT_DIR 5
#define SYS_CAT_FIRST_FILE 6

#endif
