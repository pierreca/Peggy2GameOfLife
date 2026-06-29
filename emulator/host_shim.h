// Host compat shim for the terminal emulator, force-included into
// StepCounter.cpp (which is shared verbatim with the firmware).
//
// StepCounter::counterToString() calls utoa(), an AVR libc extension that does
// not exist in a desktop libc. The test suite stubs it the same way (see
// test/host_compat.h); the emulator does NOT want the malloc-counting wrapper
// that test needs, only the missing function.
#ifndef EMULATOR_HOST_SHIM_H
#define EMULATOR_HOST_SHIM_H

#include <stdio.h>

// Minimal base-10 utoa stand-in (StepCounter only ever uses base 10).
static inline char *shim_utoa(unsigned int value, char *buffer, int base)
{
  (void)base;
  snprintf(buffer, 12, "%u", value); // unsigned int is at most 10 digits + null
  return buffer;
}

#define utoa shim_utoa

#endif
