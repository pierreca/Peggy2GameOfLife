# Host test runner for the Conway engine (issue #6).
#
#   make            build and run the tests under UndefinedBehaviorSanitizer
#   make test-asan  same, but also under AddressSanitizer (see note below)
#   make clean      remove build artifacts
#
# Builds the unmodified engine against the shared host harness (HostGrid,
# selected via -DCONWAY_HOST_BUILD; issue #5). `make` exits non-zero if any
# check fails.
#
# Note: the default target uses UBSan only. AddressSanitizer is the ideal
# extra net and `make test-asan` adds it, but some environments ship an ASan
# runtime that hangs at process start (unrelated to these tests); use
# test-asan where ASan is known good.

# := (not ?=) so the default actually takes: GNU make predefines CXX, which
# ?= would never override. Command-line `make CXX=g++` still wins over :=.
CXX      := clang++
CXXFLAGS ?= -std=c++17 -g -O1 -Wall -Wextra -fno-omit-frame-pointer
SANITIZE ?= -fsanitize=undefined
RUNENV   ?=
# CONWAY_HOST_BUILD selects the in-memory HostGrid from the shared harness;
# -I.. finds the engine/grid headers, -I../host finds HostGrid.h.
DEFINES  := -DCONWAY_HOST_BUILD
INCLUDES := -I.. -I../host

ENGINE   := ../Peggy2ConwayEngine.cpp
TESTS    := conway_tests.cpp
DEPS     := $(TESTS) $(ENGINE) ../host/HostGrid.h ../Peggy2ConwayEngine.h
BIN      := conway_tests

.PHONY: test test-asan clean
test: $(BIN)
	$(RUNENV) ./$(BIN)

# detect_leaks=0: the engine has no destructor (an upstream concern, not these
# tests), so LeakSanitizer — on by default on Linux — would otherwise fail the
# run on its known leaks rather than on a test result.
test-asan: clean
	$(MAKE) test SANITIZE="-fsanitize=address,undefined" RUNENV="ASAN_OPTIONS=detect_leaks=0"

$(BIN): $(DEPS)
	$(CXX) $(CXXFLAGS) $(SANITIZE) $(DEFINES) $(INCLUDES) $(TESTS) $(ENGINE) -o $(BIN)

clean:
	rm -rf $(BIN) $(BIN).dSYM
