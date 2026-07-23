#include <amtest.h>

void clock_once() {
  AM_TIMER_UPTIME_T uptime = io_read(AM_TIMER_UPTIME);
  printf("uptime=%llu us\n", (unsigned long long)uptime.us);
}
