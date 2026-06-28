// Host test for StepCounter::counterToString buffer sizing (issue #2).
//
// The bug is a heap-buffer-overflow: counterToString under-sizes its malloc
// at the 10/100/1000/10000 boundaries (and for any value wider than the
// hand-rolled length math allows). itoa then writes past the allocation.
//
// AddressSanitizer would normally catch this, and the file is compiled with
// -fsanitize=address. But to stay reliable across environments this test ALSO
// detects the overflow deterministically: it intercepts the malloc done inside
// counterToString and surrounds the allocation with a canary guard region.
// If itoa writes past the requested size, it stomps the guard -- which is
// exactly the overflow, caught without relying on the ASan runtime.
//
// counterToString is private; expose it for the test only.
#define private public
#include "../StepCounter.h"
#undef private

#include <cstdio>
#include <cstring>
#include <cstdlib>

// itoa is an Arduino/AVR libc extension, not part of the host stdlib. Shim it
// for host compilation. avr-libc's itoa renders base 10 as SIGNED (it prepends
// '-' for negative values), so the widest string a counter can produce is the
// most-negative int (e.g. "-2147483648"), which is what the buffer must hold.
extern "C" char *itoa(int value, char *str, int base)
{
	(void)base; // tests only use base 10
	char digits[16];
	int n = snprintf(digits, sizeof(digits), "%d", value);
	memcpy(str, digits, (size_t)n + 1); // unbounded write incl. null, like itoa
	return str;
}

// --- malloc interceptor with a canary guard region ---------------------------
static const size_t kGuard = 32;
static const unsigned char kCanary = 0xAB;
static size_t g_lastRequest = 0;

static void *tracked_malloc(size_t n)
{
	g_lastRequest = n;
	unsigned char *p = (unsigned char *)malloc(n + kGuard); // real malloc here
	if (p) memset(p + n, kCanary, kGuard);                  // poison guard bytes
	return p;
}

static bool guard_intact(const unsigned char *p, size_t n)
{
	for (size_t i = 0; i < kGuard; i++) {
		if (p[n + i] != kCanary) return false;
	}
	return true;
}

// From here on, every malloc() (including the one inside StepCounter.cpp) is
// routed through tracked_malloc.
#define malloc(n) tracked_malloc(n)
#include "../StepCounter.cpp"

static int failures = 0;

static void check(unsigned int value)
{
	StepCounter sc;
	char *got = sc.counterToString(value);
	size_t requested = g_lastRequest;

	char expected[16];
	snprintf(expected, sizeof(expected), "%d", (int)value); // itoa is signed base 10

	bool ok = true;
	if (strcmp(got, expected) != 0) {
		printf("FAIL: counterToString(%u) = \"%s\", expected \"%s\"\n",
		       value, got, expected);
		ok = false;
	}
	// The guard fires whenever the allocation is too small for the string
	// itoa wrote (which needs strlen(expected) + 1 bytes) -- i.e. the overflow.
	if (!guard_intact((unsigned char *)got, requested)) {
		printf("FAIL: counterToString(%u) overran its %zu-byte buffer "
		       "(needs %zu)\n", value, requested, strlen(expected) + 1);
		ok = false;
	}
	if (ok) {
		printf("ok:   counterToString(%u) = \"%s\" (%zu bytes)\n",
		       value, got, requested);
	} else {
		failures++;
	}
	free(got);
}

int main()
{
	const unsigned int signBit = 1u << (sizeof(unsigned int) * 8 - 1);
	unsigned int cases[] = {
		0, 9, 10, 99, 100, 999, 1000, 9999, 10000,
		signBit - 1, // INT_MAX  -> widest positive output
		signBit,     // INT_MIN  -> widest output overall (has '-')
	};
	for (unsigned int i = 0; i < sizeof(cases) / sizeof(cases[0]); i++) {
		check(cases[i]);
	}

	if (failures) {
		printf("\n%d failure(s)\n", failures);
		return 1;
	}
	printf("\nAll checks passed\n");
	return 0;
}
