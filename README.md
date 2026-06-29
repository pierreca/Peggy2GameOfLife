This small project is a naive implementation of [Conway's Game of Life](https://en.wikipedia.org/wiki/Conway%27s_Game_of_Life) on a [Peggy 2](http://shop.evilmadscientist.com/productsmenu/75) board.

![Image of the Peggy 2 board](https://raw.githubusercontent.com/pierreca/Peggy2GameOfLife/master/pictures/peggy2frame.jpg)

# How it works
The algorithm starts as soon as the Peggy 2 board is powered. On startup it seeds the random number generator from electrical noise on an unconnected analog pin (so each power-on differs), randomly initializes the first frame, then runs Conway's rules on each cell.
A loop detector stops the iterations when it detects that the life-form has returned to an earlier state — i.e. it's stuck in a repeating cycle (we'll get back to that).
Once it stops, it will display how many steps were taken to get there, as well as the maximum number of steps ever reached since the Peggy board has been powered on, then reseed and start over.

# Building and running

## On the Peggy 2 (the real thing)
You will need the **Peggy2Serial** and **SimpleTimer** libraries in your Arduino libraries folder (the sketch also uses `PeggyWriter`, which ships with Peggy2Serial). To install Peggy2Serial, see the [Peggy 2 product page](http://shop.evilmadscientist.com/productsmenu/75) and the [associated blog post](http://www.evilmadscientist.com/2008/peggy-version-2-0/). Then open `Peggy2GameOfLife.ino` in the Arduino IDE, select the Peggy 2 board/port, and upload.

## On your computer (no hardware needed)
The simulation engine is decoupled from the Peggy 2 hardware (see [Architecture](#architecture)), so the Conway logic compiles and runs on a normal machine with nothing more than a C++ compiler (clang or gcc) and `make` — no Arduino toolchain, no Peggy2Serial. This is what the tests use.

### Watch it run in your terminal
A small host-only visualizer runs the **real** engine against `HostGrid` and renders the 25×25 grid to the terminal, one generation at a time. It mirrors the sketch's main loop (seed → step → loop-detect → step-counter screen → reseed), so you can watch patterns evolve without flashing the board:

```
make -C emulator run                              # random seed, watchable speed
./emulator/emulator --pattern glider --max-gen 60  # gliders translate forever; reseed after 60 gens
./emulator/emulator --pattern blinker --seed 1 --once   # reproducible; stop at the first loop
```

Flags: `--pattern <random|blinker|glider|rpentomino>`, `--delay <ms>`, `--seed <n>` (reproducible run), `--max-gen <n>` (reseed even without a loop), `--once`. Ctrl-C exits cleanly. It cannot reproduce display **flicker** — that is a hardware multiplex / persistence-of-vision property (`RefreshAll` is a no-op on the host).

# Running the tests
All tests are self-contained host C++ — no hardware, no third-party libraries. From the repo root:

| Suite | Command |
| --- | --- |
| Engine smoke test | `make -C host test` |
| Engine unit tests (blinker, block, glider, R-pentomino, loop detection) | `make -C test` |
| Toroidal edge-wrap | `sh test/run.sh` |
| `StepCounter` heap-leak regression | `sh test/run_step_counter.sh` |
| RNG seeding | `c++ -std=c++11 -Wall tests/rng_seed_test.cpp -o /tmp/rng && /tmp/rng` |

Some suites can also build under AddressSanitizer (`sh test/run.sh`, `make -C test test-asan`). If the ASan runtime misbehaves in your environment, the default UndefinedBehaviorSanitizer / plain builds carry the same assertions, so the suites still verify correctly without it.

# Architecture
The simulation engine (`ConwayEngine`) is independent of the display hardware. It talks to a thin `ConwayGrid` abstraction — a compile-time typedef (no virtual dispatch, no per-cell cost) — that resolves to one of two backends:

- **`Peggy2Grid`** — a fully-inlined wrapper around a real Peggy 2 framebuffer, used on-device.
- **`HostGrid`** — a plain in-memory bitmap with no Arduino dependency, used for host builds and tests (selected with `-DCONWAY_HOST_BUILD`).

This is what lets the exact same engine code run on a development machine for testing while keeping on-device behavior — and performance — unchanged.

# Implementation details and conventions

## The infinite canvas
Conway's rules suppose that they run on an infinite canvas. To simulate this on a 25x25 matrix, we "loop" around the edges:
the cell at (0,0) is considered neighbors with (0,1), (1,1), (1,0) as well as (24,0), (24,1), (0,24), (1,24), and (24,24). The wrap arithmetic uses explicit modulo over the grid dimensions, so it behaves identically on the 16-bit-`int` AVR and on a 32-bit-`int` host.

This should be made an optional parameter as it helps creating undetectable loops.

## How to detect when to stop
The rules engine remembers a compact 32-bit *hash* of each recent generation (the history depth is a parameter of the engine). At each iteration it hashes the new frame and compares it against the remembered hashes: a match means the simulation has returned to an earlier state — i.e. it's stuck in a loop — so it stops.

Storing a hash instead of a whole frame makes a deep history cheap, so the engine catches much longer oscillator periods than the original 4-frame design: the firmware keeps 32 generations and the host emulator 128 — enough to catch, for example, a glider's 100-generation recurrence as it wraps around the toroidal grid. The cost is a roughly 2⁻³² chance that a hash collision ends a run one generation early, which for an autonomous display just means an occasional slightly-early reseed.

It still won't catch periods longer than the history window. The remaining ideas:
* detect an unusually large number of steps (a max-generation watchdog — issue #8)
* treat a long stretch of unchanged live-cell population as "stuck" (the open second half of issue #9)
* actively scan for gliders (computationally very expensive)
* Renounce the infinite canvas logic (and consider all cells outside of the matrix are dead). the glider would eventually exit.

## Stepping speed
The time to progress from the current step to the next is defined in the code (`STEP_PERIOD_MS`, currently 500 ms) and can be accelerated or slowed down to accomodate for modifications of the above conventions, for example, if we want to compare more frames.

## Keeping the display lit during compute
The Peggy 2 is a multiplexed (persistence-of-vision) matrix: it must be refreshed continuously or the LEDs go dark. Because the per-step timer callback runs synchronously inside `loop()`, the engine refreshes the currently-displayed frame once per row while it computes the next generation, so the picture stays lit during the otherwise-blocking compute pass.
