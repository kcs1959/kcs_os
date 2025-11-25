#define USER_BASE 0x100000

__attribute__((naked)) void user_entry(void) {
  __asm__ __volatile__("csrw sepc, %[sepc]\n"
                       "csrw sstatus, %[sstatus]\n"
                       "sret\n"
                       :
                       : [sepc] "r"(USER_BASE), [sstatus] "r"(SSTATUS_SPIE));
}

/*

*/