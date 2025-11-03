/*
OSへの割り込み（Interrupt）が発生するまで、CPUをスリープ状態にする
インラインアセンブリで、wfi(wait for interrupt)
*/

/*
C言語の無限ループ
for (;;) { ... }
while (1) { ... }
*/

void kernel_main(void) {
  // init_system();
  while (1) {
    __asm__ __volatile__("wfi");
  }
}