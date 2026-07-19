#include "trap.h"

static volatile int result[6];

int main() {
  asm volatile(
    "li t0, 7\n"
    "addi t1, t0, 5\n"
    "add t2, t1, t0\n"
    "mul t3, t2, t1\n"
    "add t4, t3, t2\n"
    "sw t4, 0(%0)\n"

    "addi t0, zero, 1\n"
    "addi t0, t0, 2\n"
    "addi t1, t0, 3\n"
    "sw t1, 4(%0)\n"

    "lw t0, 0(%0)\n"
    "addi t1, t0, 1\n"
    "sw t1, 8(%0)\n"

    "li t2, 81\n"
    "li t3, 9\n"
    "div t4, t2, t3\n"
    "addi t5, t4, 1\n"
    "sw t5, 12(%0)\n"

    "li t0, 1\n"
    "addi t1, t0, 1\n"
    "beq t1, t0, 1f\n"
    "addi t2, zero, 3\n"
    "1: sw t2, 16(%0)\n"
    :
    : "r"(result)
    : "t0", "t1", "t2", "t3", "t4", "t5", "memory"
  );

  check(result[0] == 247);
  check(result[1] == 6);
  check(result[2] == 248);
  check(result[3] == 10);
  check(result[4] == 3);
  return 0;
}
