#ifndef __TRAP_QEMU_H__
#define __TRAP_QEMU_H__

#include <stdio.h>
#include <stdlib.h>
#include <stdint.h>
#include <stdbool.h>
#include <string.h>
#include <assert.h>

// Macro from klib-macros.h
#define LENGTH(arr) (sizeof(arr) / sizeof((arr)[0]))

// Simple check function
__attribute__((noinline))
void check(bool cond) {
  if (!cond) {
    printf("Check failed!\n");
    exit(1);
  }
}

// Simple halt function
__attribute__((noinline))
void halt(int code) {
  exit(code);
}

#endif
