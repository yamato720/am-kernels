#include <stdint.h>
#include "trap.h"

int main() {
  volatile uint32_t value = 0;

  value = 0x12345678;
  // FENCE is part of the base I ISA and therefore needs no Zifencei opt-in.
  asm volatile("fence rw,rw" ::: "memory");
  check(value == 0x12345678);
  return 0;
}
