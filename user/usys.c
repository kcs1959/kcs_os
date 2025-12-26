#include "usys.h"
#include "../common/common.h"

extern char __stack_top[];

__attribute__((section(".text.start"))) __attribute__((naked)) void
start(void) {
  __asm__ __volatile__("mv sp, %[stack_top]\n"
                       "call main\n"
                       "call exit\n" ::[stack_top] "r"(__stack_top));
}

int syscall(int sysno, int arg0, int arg1, int arg2) {
  register int a0 __asm__("a0") = arg0;
  register int a1 __asm__("a1") = arg1;
  register int a2 __asm__("a2") = arg2;
  register int a3 __asm__("a3") = sysno;

  __asm__ __volatile__("ecall"
                       : "=r"(a0)
                       : "r"(a0), "r"(a1), "r"(a2), "r"(a3)
                       : "memory");

  return a0;
}

void putchar(char ch) { syscall(SYS_PUTCHAR, ch, 0, 0); }
void __common_putc(char ch) __attribute__((alias("putchar")));

int getchar(void) { return syscall(SYS_GETCHAR, 0, 0, 0); }

__attribute__((noreturn)) void exit(int status) {
  syscall(SYS_EXIT, status, 0, 0);
  for (;;)
    ; // 念のため
}

void sys_list_root_dir(void) { syscall(SYS_LIST_ROOT_DIR, 0, 0, 0); }

void sys_concat_first_file(void) { syscall(SYS_CAT_FIRST_FILE, 0, 0, 0); }

int vprintf(void (*putc)(char), const char *fmt, va_list vargs);
int printf(const char *fmt, ...) {
  va_list vargs;
  va_start(vargs, fmt);
  int ret = vprintf(putchar, fmt, vargs);
  va_end(vargs);
  return ret;
}

void sys_shutdown(void) { syscall(SYS_SHUTDOWN, 0, 0, 0); }

#define USER_OPEN_FILES 8
static FILE file_table[USER_OPEN_FILES];
static bool file_table_initialized;

static void init_file_table(void) {
  if (file_table_initialized)
    return;
  for (int i = 0; i < USER_OPEN_FILES; i++)
    file_table[i].fd = -1;
  file_table_initialized = true;
}

FILE *fopen(const char *path, const char *mode) {
  init_file_table();
  int fd = syscall(SYS_FOPEN, (int)path, (int)mode, 0);
  if (fd < 0)
    return NULL;

  for (int i = 0; i < USER_OPEN_FILES; i++) {
    if (file_table[i].fd < 0) {
      file_table[i].fd = fd;
      return &file_table[i];
    }
  }

  syscall(SYS_FCLOSE, fd, 0, 0);
  return NULL;
}

int fclose(FILE *fp) {
  if (!fp || fp->fd < 0)
    return -1;

  int ret = syscall(SYS_FCLOSE, fp->fd, 0, 0);
  fp->fd = -1;
  return ret;
}

int fgetc(FILE *fp) {
  if (!fp || fp->fd < 0)
    return EOF;
  return syscall(SYS_FGETC, fp->fd, 0, 0);
}

int fputc(FILE *fp, int ch) {
  if (!fp || fp->fd < 0)
    return -1;
  return syscall(SYS_FPUTC, fp->fd, ch, 0);
}
