# Host test for StepCounter (issue #2). StepCounter.{h,cpp} only depend on
# <stdlib.h>, so they build and run on the host.
#
#   make test   # build + run the buffer-sizing test
#   make asan   # build with AddressSanitizer (defense in depth)
#
# The test detects the heap-buffer-overflow deterministically via a malloc
# canary, so it does not rely on a working ASan runtime.
CXX ?= clang++
CXXFLAGS ?= -std=c++11 -g -Wall -Wextra

test: test_counterToString.cpp ../StepCounter.cpp ../StepCounter.h
	$(CXX) $(CXXFLAGS) -o test_counterToString test_counterToString.cpp
	./test_counterToString

asan: test_counterToString.cpp ../StepCounter.cpp ../StepCounter.h
	$(CXX) $(CXXFLAGS) -fsanitize=address -o test_counterToString_asan test_counterToString.cpp
	./test_counterToString_asan

clean:
	rm -f test_counterToString test_counterToString_asan

.PHONY: test asan clean
