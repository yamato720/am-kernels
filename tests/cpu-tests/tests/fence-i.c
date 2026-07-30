#include <stdint.h>
#include "trap.h"

typedef int (*generated_function_t)(void);

static uint32_t generated_code[] __attribute__((aligned(16))) = {
  0x00100513, // addi a0, zero, 1
  0x00008067  // ret
};

int main() {
  generated_function_t generated =
    (generated_function_t)(uintptr_t)generated_code;

  check(generated() == 1);
  generated_code[0] = 0x00200513; // addi a0, zero, 2
  asm volatile(".word 0x0000100f" ::: "memory"); // fence.i
  check(generated() == 2);
  return 0;
}
