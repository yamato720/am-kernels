#include "trap.h"

__attribute__((aligned(64), naked, noinline))
static void csr_then_cold_fetch(void) {
  asm volatile(
    ".rept 15\n"
    "nop\n"
    ".endr\n"
    "csrr zero, mstatus\n"
    "ret\n"
  );
}

int main(void) {
  csr_then_cold_fetch();
  check(1);
  return 0;
}
