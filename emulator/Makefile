# Terminal emulator / visualizer for the Conway engine (issue #27).
#
#   make -C emulator        # build ./emulator/emulator
#   make -C emulator run     # build and run it (random seed, watchable speed)
#   make -C emulator clean    # remove build artifacts
#
# Like host/Makefile, CONWAY_HOST_BUILD swaps in HostGrid via ConwayGrid.h and
# -I.. lets the emulator find the engine + shared headers at the repo root.
# StepCounter.cpp is shared verbatim with the firmware and calls AVR's utoa(),
# so it is force-compiled with host_shim.h (-include) to supply that symbol.

CXX ?= c++
CXXFLAGS ?= -std=c++11 -Wall -Wextra -O2
CPPFLAGS += -DCONWAY_HOST_BUILD -I. -I.. -I../host
LDFLAGS +=
# std::thread needs the pthread runtime on Linux; harmless on macOS/clang.
LDLIBS += -lpthread

BIN := emulator
HDRS := host_shim.h ../ConwayEngine.h ../ConwayGrid.h ../GridDimensions.h \
        ../host/HostGrid.h ../StepCounter.h

OBJS := emulator.o conwayengine.o stepcounter.o

.PHONY: all run clean

all: $(BIN)

$(BIN): $(OBJS)
	$(CXX) $(CXXFLAGS) $(OBJS) -o $(BIN) $(LDFLAGS) $(LDLIBS)

emulator.o: emulator.cpp $(HDRS)
	$(CXX) $(CXXFLAGS) $(CPPFLAGS) -c emulator.cpp -o emulator.o

conwayengine.o: ../ConwayEngine.cpp $(HDRS)
	$(CXX) $(CXXFLAGS) $(CPPFLAGS) -c ../ConwayEngine.cpp -o conwayengine.o

# StepCounter is shared with the firmware; -include supplies the AVR utoa shim.
stepcounter.o: ../StepCounter.cpp ../StepCounter.h host_shim.h
	$(CXX) $(CXXFLAGS) $(CPPFLAGS) -include host_shim.h -c ../StepCounter.cpp -o stepcounter.o

run: $(BIN)
	./$(BIN)

clean:
	rm -f $(BIN) $(OBJS)
