# Host build for the Conway engine: compiles and tests the engine WITHOUT the
# Arduino toolchain by swapping the Peggy2-backed grid for an in-memory one.
#
#   make -C host test    # build and run the smoke test
#   make -C host clean    # remove build artifacts
#
# CONWAY_HOST_BUILD selects HostGrid via ConwayGrid.h. -I.. lets the host
# sources find the engine + shared grid headers that live at the repo root.

CXX ?= c++
CXXFLAGS ?= -std=c++11 -Wall -Wextra -O2
CPPFLAGS += -DCONWAY_HOST_BUILD -I. -I..

SRCS := smoke_test.cpp ../Peggy2ConwayEngine.cpp
BIN := smoke_test

.PHONY: test clean

$(BIN): $(SRCS) HostGrid.h ../ConwayGrid.h ../GridDimensions.h ../Peggy2ConwayEngine.h
	$(CXX) $(CXXFLAGS) $(CPPFLAGS) $(SRCS) -o $(BIN)

test: $(BIN)
	./$(BIN)

clean:
	rm -f $(BIN)
