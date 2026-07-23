#include <amtest.h>

void unaligned_load() {
  static volatile uint32_t words[2] __attribute__((aligned(4))) = { 0x11223344, 0x55667788 };
  uint32_t value;

  printf("starting intentional unaligned load\n");
  asm volatile ("lw %0, 1(%1)" : "=r"(value) : "r"(words) : "memory");
  printf("unexpected unaligned value=%u\n", value);
}
