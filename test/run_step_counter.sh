#!/bin/sh
# Host leak-regression test for issue #1. Compiles StepCounter.cpp on the host
# (it only depends on <stdlib.h>) with a counting malloc wrapper and runs the
# assertion harness. Exits non-zero if a per-call heap leak is present.
set -e
cd "$(dirname "$0")/.."
clang++ -std=c++17 -Wall -c StepCounter.cpp -include test/host_compat.h -o test/stepcounter.o
clang++ -std=c++17 -Wall test/test_step_counter.cpp test/stepcounter.o -o test/runner
./test/runner
