#include "trap.h"

static volatile int integer_a = 84;
static volatile int integer_b = 7;
static volatile int integer_results[5];
static volatile float floating_a = 9.0f;
static volatile float floating_b = 3.0f;
static volatile float floating_results[5];

asm(
  ".globl main\n"
  ".type main, @function\n"
  "main:\n"
  "  li t0, 0x2000\n"
  "  csrs mstatus, t0\n"
  "  j fpga_fallback_main\n"
);

__attribute__((noinline))
static void integer_four_operations(int a, int b) {
  integer_results[0] = a + b;
  integer_results[1] = a - b;
  integer_results[2] = a * b;
  integer_results[3] = a / b;
  integer_results[4] = (integer_results[0] - integer_results[1]) * integer_results[3] / 2;
}

__attribute__((noinline))
static void floating_four_operations(float a, float b) {
  floating_results[0] = a + b;
  floating_results[1] = a - b;
  floating_results[2] = a * b;
  floating_results[3] = a / b;
  floating_results[4] =
    (floating_results[0] - floating_results[1]) * floating_results[3] / b;
}

int fpga_fallback_main() {
  integer_four_operations(integer_a, integer_b);
  floating_four_operations(floating_a, floating_b);

  check(integer_results[0] == 91);
  check(integer_results[1] == 77);
  check(integer_results[2] == 588);
  check(integer_results[3] == 12);
  check(integer_results[4] == 84);
  check(floating_results[0] == 12.0f);
  check(floating_results[1] == 6.0f);
  check(floating_results[2] == 27.0f);
  check(floating_results[3] == 3.0f);
  check(floating_results[4] == 6.0f);
  return 0;
}
