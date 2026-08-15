#include "trap.h"

int main(void) {
  asm volatile(
    "li t0, 1024\n"
    "1:\n"
    ".rept 16\n"
    "addi t1, t1, 1\n"
    "addi t2, t2, 1\n"
    "addi t3, t3, 1\n"
    "addi t4, t4, 1\n"
    "addi t5, t5, 1\n"
    "addi t6, t6, 1\n"
    "addi a1, a1, 1\n"
    "addi a2, a2, 1\n"
    "addi a3, a3, 1\n"
    "addi a4, a4, 1\n"
    "addi a5, a5, 1\n"
    "addi a6, a6, 1\n"
    "addi a7, a7, 1\n"
    "addi t1, t1, 1\n"
    "addi t2, t2, 1\n"
    "addi t3, t3, 1\n"
    "addi t4, t4, 1\n"
    "addi t5, t5, 1\n"
    "addi t6, t6, 1\n"
    "addi a1, a1, 1\n"
    "addi a2, a2, 1\n"
    "addi a3, a3, 1\n"
    "addi a4, a4, 1\n"
    "addi a5, a5, 1\n"
    "addi a6, a6, 1\n"
    "addi a7, a7, 1\n"
    ".endr\n"
    "addi t0, t0, -1\n"
    "bnez t0, 1b\n"
    :
    :
    : "t0", "t1", "t2", "t3", "t4", "t5", "t6",
      "a1", "a2", "a3", "a4", "a5", "a6", "a7"
  );
  return 0;
}
