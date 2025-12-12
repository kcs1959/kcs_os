#include "kernel.h"
#include "fat16.h"
#include "virtio.h"

typedef unsigned char uint8_t;
typedef unsigned int uint32_t;
typedef uint32_t size_t;

extern char __kernel_base[];
extern char __bss[], __bss_end[], __stack_top[];
extern char __free_ram[], __free_ram_end[];
extern char _binary_shell_bin_start[], _binary_shell_bin_size[];

static int kfopen(const char *path, const char *mode);
static int kfclose(int fd);
static int kfgetc(int fd);
static int kfputc(int fd, int ch);

struct process procs[PROCS_MAX];
struct process *current_proc;
struct process *idle_proc;

// Simple Memory Allocaltion
paddr_t alloc_pages(uint32_t n) {
  static paddr_t next_paddr = (paddr_t)__free_ram;
  paddr_t paddr = next_paddr;
  next_paddr += n * PAGE_SIZE;

  if (next_paddr > (paddr_t)__free_ram_end) {
    PANIC("out of memory!!!!!");
  }

  memset((void *)paddr, 0, n * PAGE_SIZE);
  return paddr;
}

// Virtual Memory
void map_page(uint32_t *table1, uint32_t vaddr, paddr_t paddr, uint32_t flags) {
  if (!is_aligned(vaddr, PAGE_SIZE))
    PANIC("unaligned vaddr %x", vaddr);

  if (!is_aligned(paddr, PAGE_SIZE))
    PANIC("unaligned paddr %x", paddr);

  uint32_t vpn1 = (vaddr >> 22) & 0x3ff;
  if ((table1[vpn1] & PAGE_V) == 0) {
    // 1段目のページテーブル作成
    uint32_t pt_paddr = alloc_pages(1);
    table1[vpn1] = ((pt_paddr / PAGE_SIZE) << 10) | PAGE_V;
  }

  // 2段目のページテーブルにエントリを追加する
  uint32_t vpn0 = (vaddr >> 12) & 0x3ff;
  uint32_t *table0 = (uint32_t *)((table1[vpn1] >> 10) * PAGE_SIZE);
  table0[vpn0] = ((paddr / PAGE_SIZE) << 10) | flags | PAGE_V;
}

__attribute__((naked)) void user_entry(void) {
  __asm__ __volatile__("csrw sepc, %[sepc]\n"
                       "csrw sstatus, %[sstatus]\n"
                       "sret\n"
                       :
                       : [sepc] "r"(USER_BASE), [sstatus] "r"(SSTATUS_SPIE));
}

// Processes
struct process *create_process(const void *image, size_t image_size) {
  struct process *proc = NULL;
  int i;
  for (i = 0; i < PROCS_MAX; i++) {
    if (procs[i].state == PROC_UNUSED) {
      proc = &procs[i];
      break;
    }
  }

  if (!proc)
    PANIC("no free process slots");

  uint32_t *sp = (uint32_t *)&proc->stack[sizeof(proc->stack)];
  *--sp = 0;                    // s11
  *--sp = 0;                    // s10
  *--sp = 0;                    // s9
  *--sp = 0;                    // s8
  *--sp = 0;                    // s7
  *--sp = 0;                    // s6
  *--sp = 0;                    // s5
  *--sp = 0;                    // s4
  *--sp = 0;                    // s3
  *--sp = 0;                    // s2
  *--sp = 0;                    // s1
  *--sp = 0;                    // s0
  *--sp = (uint32_t)user_entry; // ra

  uint32_t *page_table = (uint32_t *)alloc_pages(1);

  // カーネルのページをマッピングする
  for (paddr_t paddr = (paddr_t)__kernel_base; paddr < (paddr_t)__free_ram_end;
       paddr += PAGE_SIZE)
    map_page(page_table, paddr, paddr, PAGE_R | PAGE_W | PAGE_X);

  // MMIO領域をマッピングする
  map_page(page_table, VIRTIO_BLK_PADDR, VIRTIO_BLK_PADDR, PAGE_R | PAGE_W);

  // ユーザーのページをマッピングする
  for (uint32_t off = 0; off < image_size; off += PAGE_SIZE) {
    paddr_t page = alloc_pages(1);

    // コピーするデータがページサイズより小さい場合を考慮
    size_t remaining = image_size - off;
    size_t copy_size = PAGE_SIZE <= remaining ? PAGE_SIZE : remaining;

    // 確保したページにデータをコピー
    memcpy((void *)page, image + off, copy_size);

    // ページテーブルにマッピング
    map_page(page_table, USER_BASE + off, page,
             PAGE_U | PAGE_R | PAGE_W | PAGE_X);
  }

  // 各フィールドを初期化
  proc->pid = i + 1;
  proc->state = PROC_RUNNABLE;
  proc->sp = (uint32_t)sp;
  proc->page_table = page_table;
  return proc;
}

__attribute__((naked)) void switch_context(uint32_t *prev_sp,
                                           uint32_t *next_sp) {
  __asm__ __volatile__(
      // 実行中プロセスのスタックへレジスタを保存
      "addi sp, sp, -13 * 4\n"
      "sw ra,  0  * 4(sp)\n"
      "sw s0,  1  * 4(sp)\n"
      "sw s1,  2  * 4(sp)\n"
      "sw s2,  3  * 4(sp)\n"
      "sw s3,  4  * 4(sp)\n"
      "sw s4,  5  * 4(sp)\n"
      "sw s5,  6  * 4(sp)\n"
      "sw s6,  7  * 4(sp)\n"
      "sw s7,  8  * 4(sp)\n"
      "sw s8,  9  * 4(sp)\n"
      "sw s9,  10 * 4(sp)\n"
      "sw s10, 11 * 4(sp)\n"
      "sw s11, 12 * 4(sp)\n"

      // スタックポインタの切り替え
      "sw sp, (a0)\n"
      "lw sp, (a1)\n"

      // 次のプロセスのスタックからレジスタを復元
      "lw ra,  0  * 4(sp)\n"
      "lw s0,  1  * 4(sp)\n"
      "lw s1,  2  * 4(sp)\n"
      "lw s2,  3  * 4(sp)\n"
      "lw s3,  4  * 4(sp)\n"
      "lw s4,  5  * 4(sp)\n"
      "lw s5,  6  * 4(sp)\n"
      "lw s6,  7  * 4(sp)\n"
      "lw s7,  8  * 4(sp)\n"
      "lw s8,  9  * 4(sp)\n"
      "lw s9,  10 * 4(sp)\n"
      "lw s10, 11 * 4(sp)\n"
      "lw s11, 12 * 4(sp)\n"
      "addi sp, sp, 13 * 4\n"
      "ret\n");
}

void yield(void) {
  // 実行可能なプロセスを探す
  struct process *next = idle_proc;
  for (int i = 0; i < PROCS_MAX; i++) {
    struct process *proc = &procs[(current_proc->pid + i) % PROCS_MAX];
    if (proc->state == PROC_RUNNABLE && proc->pid > 0) {
      next = proc;
      break;
    }
  }

  // 現在実行中のプロセス以外に、実行可能なプロセスがない。戻って処理を続行する
  if (next == current_proc)
    return;

  // プロセス切り替え時、sscratchレジスタに、実行中プロセスのカーネルスタックの初期値を与える
  __asm__ __volatile__(
      "sfence.vma\n"
      "csrw satp, %[satp]\n"
      "sfence.vma\n"
      "csrw sscratch, %[sscratch]\n"
      :
      : [satp] "r"(SATP_SV32 | ((uint32_t)next->page_table / PAGE_SIZE)),
        [sscratch] "r"((uint32_t)&next->stack[sizeof(next->stack)]));

  // コンテキストスイッチ
  struct process *prev = current_proc;
  current_proc = next;
  switch_context(&prev->sp, &next->sp);
}

// sbi_call
struct sbiret sbi_call(long arg0, long arg1, long arg2, long arg3, long arg4,
                       long arg5, long fid, long eid) {
  register long a0 __asm__("a0") = arg0;
  register long a1 __asm__("a1") = arg1;
  register long a2 __asm__("a2") = arg2;
  register long a3 __asm__("a3") = arg3;
  register long a4 __asm__("a4") = arg4;
  register long a5 __asm__("a5") = arg5;
  register long a6 __asm__("a6") = fid;
  register long a7 __asm__("a7") = eid;

  __asm__ __volatile__("ecall"
                       : "=r"(a0), "=r"(a1)
                       : "r"(a0), "r"(a1), "r"(a2), "r"(a3), "r"(a4), "r"(a5),
                         "r"(a6), "r"(a7)
                       : "memory");
  return (struct sbiret){.error = a0, .value = a1};
}

void shutdown(void) { sbi_call(0, 0, 0, 0, 0, 0, 0, 8); }

// カーネルのデバッグ用のI/O
void kputchar(char ch) { sbi_call(ch, 0, 0, 0, 0, 0, 0, 1); }

void putchar(char ch) { kputchar(ch); }

long kgetchar(void) {
  struct sbiret ret = sbi_call(0, 0, 0, 0, 0, 0, 0, 2);
  return ret.error;
}

int kvprintf(const char *fmt, va_list vargs) {
  int count = 0;
  while (*fmt) {
    if (*fmt == '%') {
      fmt++;
      switch (*fmt) {
      case '\0':
        kputchar('%');
        count++;
        goto end;
      case '%':
        kputchar('%');
        count++;
        break;

      case 's': {
        const char *s = va_arg(vargs, const char *);
        if (!s)
          s = "(null)";
        while (*s) {
          kputchar(*s);
          s++;
          count++;
        }
        break;
      }
      case 'd': { // Print an integer in decimal.
        int value = va_arg(vargs, int);
        unsigned magnitude = value;
        if (value < 0) {
          kputchar('-');
          magnitude = -magnitude;
          count++;
        }

        unsigned divisor = 1;
        while (magnitude / divisor > 9)
          divisor *= 10;

        while (divisor > 0) {
          kputchar('0' + magnitude / divisor);
          magnitude %= divisor;
          divisor /= 10;
          count++;
        }
        break;
      }
      case 'x': { // Print an integer in hexadecimal.
        unsigned value = va_arg(vargs, unsigned);
        for (int i = 7; i >= 0; i--) {
          unsigned nibble = (value >> (i * 4)) & 0xf;
          kputchar("0123456789abcdef"[nibble]);
          count++;
        }
        break;
      }
      }
    } else {
      kputchar(*fmt);
      count++;
    }
    fmt++;
  }

end:
  return count;
}

int kprintf(const char *fmt, ...) {
  va_list vargs;
  va_start(vargs, fmt);
  int ret = kvprintf(fmt, vargs);
  va_end(vargs);
  return ret;
}

void *memcpy(void *dst, const void *src, size_t n) {
  uint8_t *d = (uint8_t *)dst;
  const uint8_t *s = (const uint8_t *)src;
  while (n--) {
    *d++ = *s++;
  }
  return dst;
}

void *memset(void *buf, int c, size_t n) {
  uint8_t *p = (uint8_t *)buf;
  while (n--)
    *p++ = c;
  return buf;
}

char *strcpy(char *dst, const char *src) {
  char *d = dst;
  while (*src) {
    *d++ = *src++;
  }
  *d = '\0';
  return dst;
}

int strcmp(const char *s1, const char *s2) {
  while (*s1 && *s2) {
    if (*s1 != *s2)
      break;
    s1++;
    s2++;
  }
  return *(unsigned char *)s1 - *(unsigned char *)s2;
}

int strncmp(const char *s1, const char *s2, uint32_t n) {
  for (uint32_t i = 0; i < n; i++) {
    if (s1[i] != s2[i] || s1[i] == '\0' || s2[i] == '\0') {
      return (unsigned char)s1[i] - (unsigned char)s2[i];
    }
  }
  return 0;
}

static unsigned long next = 1;

void srand(unsigned int seed) { next = seed; } // シード値からrandを呼び出す場合

int rand(void) {
  next = next * 1103515245 + 12345;
  return (unsigned int)(next >> 16) & 0x7fff;
}

// Interrupt
__attribute__((naked)) __attribute__((aligned(4))) void kernel_entry(void) {
  __asm__ __volatile__("csrrw sp, sscratch, sp\n"
                       "addi sp, sp, -4 * 31\n"
                       "sw ra,  4 * 0(sp)\n"
                       "sw gp,  4 * 1(sp)\n"
                       "sw tp,  4 * 2(sp)\n"
                       "sw t0,  4 * 3(sp)\n"
                       "sw t1,  4 * 4(sp)\n"
                       "sw t2,  4 * 5(sp)\n"
                       "sw t3,  4 * 6(sp)\n"
                       "sw t4,  4 * 7(sp)\n"
                       "sw t5,  4 * 8(sp)\n"
                       "sw t6,  4 * 9(sp)\n"
                       "sw a0,  4 * 10(sp)\n"
                       "sw a1,  4 * 11(sp)\n"
                       "sw a2,  4 * 12(sp)\n"
                       "sw a3,  4 * 13(sp)\n"
                       "sw a4,  4 * 14(sp)\n"
                       "sw a5,  4 * 15(sp)\n"
                       "sw a6,  4 * 16(sp)\n"
                       "sw a7,  4 * 17(sp)\n"
                       "sw s0,  4 * 18(sp)\n"
                       "sw s1,  4 * 19(sp)\n"
                       "sw s2,  4 * 20(sp)\n"
                       "sw s3,  4 * 21(sp)\n"
                       "sw s4,  4 * 22(sp)\n"
                       "sw s5,  4 * 23(sp)\n"
                       "sw s6,  4 * 24(sp)\n"
                       "sw s7,  4 * 25(sp)\n"
                       "sw s8,  4 * 26(sp)\n"
                       "sw s9,  4 * 27(sp)\n"
                       "sw s10, 4 * 28(sp)\n"
                       "sw s11, 4 * 29(sp)\n"

                       "csrr a0, sscratch\n"
                       "sw a0, 4 * 30(sp)\n"

                       "addi a0, sp, 4 * 31\n"
                       "csrw sscratch, a0\n"

                       "mv a0, sp\n"
                       "call handle_trap\n"

                       "lw ra,  4 * 0(sp)\n"
                       "lw gp,  4 * 1(sp)\n"
                       "lw tp,  4 * 2(sp)\n"
                       "lw t0,  4 * 3(sp)\n"
                       "lw t1,  4 * 4(sp)\n"
                       "lw t2,  4 * 5(sp)\n"
                       "lw t3,  4 * 6(sp)\n"
                       "lw t4,  4 * 7(sp)\n"
                       "lw t5,  4 * 8(sp)\n"
                       "lw t6,  4 * 9(sp)\n"
                       "lw a0,  4 * 10(sp)\n"
                       "lw a1,  4 * 11(sp)\n"
                       "lw a2,  4 * 12(sp)\n"
                       "lw a3,  4 * 13(sp)\n"
                       "lw a4,  4 * 14(sp)\n"
                       "lw a5,  4 * 15(sp)\n"
                       "lw a6,  4 * 16(sp)\n"
                       "lw a7,  4 * 17(sp)\n"
                       "lw s0,  4 * 18(sp)\n"
                       "lw s1,  4 * 19(sp)\n"
                       "lw s2,  4 * 20(sp)\n"
                       "lw s3,  4 * 21(sp)\n"
                       "lw s4,  4 * 22(sp)\n"
                       "lw s5,  4 * 23(sp)\n"
                       "lw s6,  4 * 24(sp)\n"
                       "lw s7,  4 * 25(sp)\n"
                       "lw s8,  4 * 26(sp)\n"
                       "lw s9,  4 * 27(sp)\n"
                       "lw s10, 4 * 28(sp)\n"
                       "lw s11, 4 * 29(sp)\n"
                       "lw sp,  4 * 30(sp)\n"
                       "sret\n");
}

// sbi legacy extension
void handle_syscall(struct trap_frame *f) {
  switch (f->a3) {
  case SYS_PUTCHAR:
    kputchar(f->a0);
    break;
  case SYS_GETCHAR:
    while (1) {
      long ch = kgetchar();
      if (ch >= 0) {
        f->a0 = ch;
        break;
      }
      yield();
    }
    break;
  case SYS_EXIT:
    kprintf("process %d exited\n", current_proc->pid);
    current_proc->state = PROC_EXITED;
    yield();
    PANIC("unreachable");
  case SYS_CREATE_FILE:
    while (1) {
      /*
      long filename = kgetchar()もどき
      create_file(filename, filenameの長さ);
      yield();
      */
    }
    break;
  case SYS_LIST_ROOT_DIR:
    fat16_list_root_dir();
    yield();
    break;
  case SYS_CAT_FIRST_FILE:
    fat16_concatenate_first_file();
    yield();
    break;
  case SYS_SHUTDOWN:
    kprintf("shutting down...\n");
    shutdown();
  case SYS_FOPEN:
    f->a0 = kfopen((const char *)f->a0, (const char *)f->a1);
    break;
  case SYS_FCLOSE:
    f->a0 = kfclose(f->a0);
    break;
  case SYS_FGETC:
    f->a0 = kfgetc(f->a0);
    break;
  case SYS_FPUTC:
    f->a0 = kfputc(f->a0, f->a1);
    break;
  default:
    PANIC("unexpected syscall a3=%x\n", f->a3);
  }
}

void handle_trap(struct trap_frame *f) {
  uint32_t scause = READ_CSR(scause);
  uint32_t stval = READ_CSR(stval);
  uint32_t user_pc = READ_CSR(sepc);

  if (scause == SCAUSE_ECALL) {
    handle_syscall(f);
    user_pc += 4;
  } else {
    PANIC("unexpected trap scause=%x, stval=%x, sepc=%x\n", scause, stval,
          user_pc);
  }

  WRITE_CSR(sepc, user_pc);
}

// I/O

// File System

static bool mode_contains(const char *mode, char token) {
  if (!mode)
    return false;
  while (*mode) {
    if (*mode == token)
      return true;
    mode++;
  }
  return false;
}

static struct dir_entry *find_dir_entry_by_path(const char *path) {
  for (int i = 0; i < BPB_RootEntCnt; i++) {
    if (root_dir[i].name[0] == 0x00)
      break;
    if (root_dir[i].name[0] == 0xE5)
      continue;

    char name[13];
    int p = 0;

    for (int j = 0; j < 8; j++) {
      if (root_dir[i].name[j] != ' ')
        name[p++] = root_dir[i].name[j];
    }

    if (root_dir[i].ext[0] != ' ') {
      name[p++] = '.';
      for (int j = 0; j < 3; j++) {
        if (root_dir[i].ext[j] != ' ')
          name[p++] = root_dir[i].ext[j];
      }
    }

    name[p] = '\0';

    if (strcmp(name, path) == 0)
      return &root_dir[i];
  }

  return NULL;
}

static uint16_t find_free_cluster(void) {
  for (uint16_t i = 2; i < FAT_ENTRY_NUM; i++) {
    if (fat[i] == 0x0000)
      return i;
  }
  return 0;
}

static int locate_cluster_for_offset(uint16_t start_cluster, uint32_t offset,
                                     uint16_t *cluster_out,
                                     uint32_t *offset_in_cluster) {
  if (!cluster_out || !offset_in_cluster)
    return -1;
  if (start_cluster < 2 || start_cluster >= FAT_ENTRY_NUM)
    return -1;

  uint16_t cluster = start_cluster;
  uint32_t remaining = offset;

  while (remaining >= CLUSTER_SIZE) {
    uint16_t next = fat[cluster];
    if (next == 0x0000 || next == 0xFFFF || next >= FAT_ENTRY_NUM)
      return -1;
    cluster = next;
    remaining -= CLUSTER_SIZE;
  }

  *cluster_out = cluster;
  *offset_in_cluster = remaining;
  return 0;
}

static int ensure_cluster_for_offset(uint16_t start_cluster, uint32_t offset,
                                     uint16_t *cluster_out,
                                     uint32_t *offset_in_cluster,
                                     bool *target_is_new) {
  if (!cluster_out || !offset_in_cluster)
    return -1;
  if (start_cluster < 2 || start_cluster >= FAT_ENTRY_NUM)
    return -1;

  bool cluster_new = false;
  if (fat[start_cluster] == 0x0000) {
    fat[start_cluster] = 0xFFFF;
    cluster_new = true;
  }

  uint16_t cluster = start_cluster;
  uint32_t remaining = offset;

  while (remaining >= CLUSTER_SIZE) {
    uint16_t next = fat[cluster];
    bool allocated = false;

    if (next == 0x0000 || next == 0xFFFF || next >= FAT_ENTRY_NUM) {
      next = find_free_cluster();
      if (next == 0)
        return -1;
      fat[cluster] = next;
      fat[next] = 0xFFFF;
      allocated = true;
    }

    cluster = next;
    remaining -= CLUSTER_SIZE;
    cluster_new = allocated;
  }

  if (target_is_new)
    *target_is_new = cluster_new;

  *cluster_out = cluster;
  *offset_in_cluster = remaining;
  return 0;
}

#define OPEN_FILES_MAX 16
struct open_file {
  struct dir_entry *entry;
  uint32_t position;
  bool used;
};

static struct open_file open_files[OPEN_FILES_MAX];

static int alloc_open_file(void) {
  for (int i = 0; i < OPEN_FILES_MAX; i++) {
    if (!open_files[i].used)
      return i;
  }
  return -1;
}

static int kfopen(const char *path, const char *mode) {
  if (!path || !mode)
    return -1;

  bool want_write = mode_contains(mode, 'w');
  bool want_append = mode_contains(mode, 'a');
  bool want_create = want_write || want_append;

  read_fat_from_disk();
  read_root_dir_from_disk();

  struct dir_entry *target = find_dir_entry_by_path(path);

  if (!target && want_create) {
    if (create_file(path, NULL, 0) < 0)
      return -1;

    read_root_dir_from_disk();
    target = find_dir_entry_by_path(path);
  }

  if (!target)
    return -1;

  if (want_write) {
    if (write_file(target->start_cluster, NULL, 0) < 0)
      return -1;
    read_root_dir_from_disk();
    target = find_dir_entry_by_path(path);
    if (!target)
      return -1;
  }

  int slot = alloc_open_file();
  if (slot < 0)
    return -1;

  open_files[slot].used = true;
  open_files[slot].entry = target;
  open_files[slot].position = want_append ? target->size : 0;
  return slot;
}

static int kfclose(int fd) {
  if (fd < 0 || fd >= OPEN_FILES_MAX || !open_files[fd].used)
    return -1;

  open_files[fd].used = false;
  open_files[fd].entry = NULL;
  open_files[fd].position = 0;
  return 0;
}

static int kfputc(int fd, int ch) {
  if (fd < 0 || fd >= OPEN_FILES_MAX || !open_files[fd].used)
    return -1;

  read_fat_from_disk();
  read_root_dir_from_disk();

  struct dir_entry *entry = open_files[fd].entry;

  uint16_t cluster;
  uint32_t offset_in_cluster;
  bool target_is_new = false;

  if (ensure_cluster_for_offset(entry->start_cluster, open_files[fd].position,
                                &cluster, &offset_in_cluster,
                                &target_is_new) < 0)
    return -1;

  uint8_t cluster_buf[CLUSTER_SIZE];
  if (target_is_new)
    memset(cluster_buf, 0, sizeof(cluster_buf));
  else
    read_cluster(cluster, cluster_buf);

  cluster_buf[offset_in_cluster] = (uint8_t)ch;
  write_cluster(cluster, cluster_buf);

  open_files[fd].position += 1;
  if (open_files[fd].position > entry->size)
    entry->size = open_files[fd].position;

  write_fat_to_disk();
  write_root_dir_to_disk();

  return ch & 0xff;
}

static int kfgetc(int fd) {
  if (fd < 0 || fd >= OPEN_FILES_MAX || !open_files[fd].used)
    return EOF;

  struct dir_entry *entry = open_files[fd].entry;
  if (open_files[fd].position >= entry->size)
    return EOF;

  read_fat_from_disk();

  uint16_t cluster;
  uint32_t offset_in_cluster;
  if (locate_cluster_for_offset(entry->start_cluster, open_files[fd].position,
                                &cluster, &offset_in_cluster) < 0)
    return EOF;

  uint8_t cluster_buf[CLUSTER_SIZE];
  read_cluster(cluster, cluster_buf);

  open_files[fd].position += 1;
  return cluster_buf[offset_in_cluster];
}

// process_switch_test
struct process *proc_a;
struct process *proc_b;

void delay(void) {
  for (int i = 0; i < 30000000; i++)
    __asm__ __volatile__("nop"); // 何もしない命令
}

void proc_a_entry(void) {
  kprintf("starting process A\n");
  while (1) {
    kputchar('A');
    yield();
    delay();
  }
}

void proc_b_entry(void) {
  kprintf("starting process B\n");
  while (1) {
    kputchar('B');
    yield();
    delay();
  }
}

// Start
__attribute__((section(".text.boot"))) __attribute__((naked)) void boot(void) {
  __asm__ __volatile__("mv sp, %[stack_top]\n"
                       "j kernel_main\n"
                       :
                       : [stack_top] "r"(__stack_top));
}

void kernel_main(void) {
  memset(__bss, 0, (size_t)__bss_end - (size_t)__bss);
  WRITE_CSR(stvec, (uint32_t)kernel_entry);
  virtio_blk_init();
  init_fat16_disk();

  idle_proc = create_process(NULL, 0);
  idle_proc->pid = 0;
  current_proc = idle_proc;

  kprintf("\n\nWelcome to KCS OS!\n");

  char buf[SECTOR_SIZE];
  read_write_disk(buf, 0, false);
  kprintf("first sector: %s\n", buf);

  create_file("test.txt", (uint8_t *)"hello", 5);
  create_file("test2.txt", (uint8_t *)"hello2", 6);

  int fd = kfopen("test.txt", "a");
  char *msg = " world!";
  for (int i = 0; msg[i] != '\0'; i++) {
    kfputc(fd, msg[i]);
  }
  kfclose(fd);

  create_process(_binary_shell_bin_start, (size_t)_binary_shell_bin_size);
  yield();
  shutdown();
  PANIC("shell discontinued");

  for (;;) {
    __asm__ __volatile__("wfi");
  }
}
