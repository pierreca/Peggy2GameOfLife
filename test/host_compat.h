// Host-only compat shim, force-included into StepCounter.cpp for the leak test.
// Provides a utoa() (AVR libc only) and a counting malloc wrapper so the test
// can prove counterToString() performs no per-call heap allocation.
#ifndef HOST_COMPAT_H
#define HOST_COMPAT_H

#include <stdlib.h>
#include <stdio.h>

// Defined by the test translation unit.
extern unsigned long g_counterMallocCount;

static inline void *counting_malloc(size_t n)
{
	g_counterMallocCount++;
	return malloc(n);
}

// Minimal base-10 utoa stand-in (StepCounter only ever uses base 10).
static inline char *shim_utoa(unsigned int value, char *buffer, int base)
{
	(void)base;
	snprintf(buffer, 12, "%u", value); // unsigned int is at most 10 digits + null
	return buffer;
}

// Redirect the calls inside StepCounter.cpp. Defined AFTER the wrappers so the
// wrapper bodies still resolve to the real libc symbols.
#define malloc counting_malloc
#define utoa shim_utoa

#endif
