// Host regression test for issue #1: StepCounter::counterToString must not
// allocate fresh heap memory on every call (the original code malloc'd a string
// per call and never freed it, leaking on the AVR's tiny SRAM).
//
// Build (see test/run_step_counter.sh):
//   clang++ -std=c++17 -Wall -c StepCounter.cpp -include test/host_compat.h -o test/stepcounter.o
//   clang++ -std=c++17 -Wall test/test_step_counter.cpp test/stepcounter.o -o test/runner
//   ./test/runner
//
// The counting malloc wrapper lives in host_compat.h and is only spliced into
// StepCounter.cpp, so g_counterMallocCount reflects allocations made by the
// counter code alone.
#include "../StepCounter.h"
#include <cstdio>
#include <cstring>

unsigned long g_counterMallocCount = 0;

int main()
{
	StepCounter sc;
	for (int i = 0; i < 1000; i++) sc.IncrementCount();

	// Correctness: the converted strings must still be right.
	if (strcmp(sc.GetCurrentCountString(), "1000") != 0)
	{
		printf("FAIL: GetCurrentCountString() = \"%s\", expected \"1000\"\n",
		       sc.GetCurrentCountString());
		return 1;
	}
	if (strcmp(sc.GetMaxCountString(), "1000") != 0)
	{
		printf("FAIL: GetMaxCountString() = \"%s\", expected \"1000\"\n",
		       sc.GetMaxCountString());
		return 1;
	}

	// The two getters must hand back distinct buffers, so a caller holding both
	// strings at once never sees one clobber the other.
	if (sc.GetCurrentCountString() == sc.GetMaxCountString())
	{
		printf("FAIL: getters returned the same buffer (aliasing)\n");
		return 1;
	}

	// The leak check: many calls must not grow the heap.
	g_counterMallocCount = 0;
	for (int i = 0; i < 10000; i++)
	{
		volatile char *a = sc.GetCurrentCountString();
		volatile char *b = sc.GetMaxCountString();
		(void)a;
		(void)b;
	}

	if (g_counterMallocCount != 0)
	{
		printf("FAIL: %lu heap allocation(s) across 20000 getter calls "
		       "(per-call leak still present)\n", g_counterMallocCount);
		return 1;
	}

	printf("PASS: 0 heap allocations across 20000 getter calls\n");
	return 0;
}
