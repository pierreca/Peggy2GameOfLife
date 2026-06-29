# CLAUDE.md

This file provides guidance to Claude Code (claude.ai/code) when working with code in this repository.

## What this is

An Arduino sketch that runs Conway's Game of Life on a [Peggy 2.0](http://shop.evilmadscientist.com/productsmenu/75) board — a 25×25 monochrome LED matrix. The sketch starts on power-up: it seeds the RNG from analog-pin noise, seeds a random first frame, iterates Conway's rules, stops when it detects the field has looped back to an earlier state, briefly displays the step count, then reseeds and repeats forever.

## Build / flash / test

There is no single command-line build for the firmware. There are two distinct build paths:

**Firmware (on-device):** compiled and uploaded with the **Arduino IDE**.
- Entry point: `Peggy2GameOfLife.ino` (provides `setup()` / `loop()`).
- Required third-party libraries in the Arduino `libraries/` folder: **Peggy2Serial** (provides `Peggy2`, `PeggyWriter`) and **SimpleTimer**.

**Host tests (off-device):** pure host C++, needs only clang/gcc + `make` — no Arduino toolchain, no Peggy2Serial. Run from the repo root:
- `make -C host test` — engine smoke test.
- `make -C test` — engine unit tests (blinker/block/glider/R-pentomino/loop detection), 24 checks, UBSan by default (`make -C test test-asan` adds ASan).
- `sh test/run.sh` — toroidal edge-wrap test.
- `sh test/run_step_counter.sh` — StepCounter heap-leak regression.
- `c++ -std=c++11 -Wall tests/rng_seed_test.cpp -o /tmp/rng && /tmp/rng` — RNG seeding.

There is also a host-only **terminal visualizer** in `emulator/` (`make -C emulator run`; flags `--pattern`/`--delay`/`--seed`/`--max-gen`/`--once`). It runs the real engine against `HostGrid` and renders each generation to the terminal, mirroring the sketch's main loop (seed → step → `DetectLoop` → counter screen → reseed). It reuses `StepCounter.cpp` verbatim, supplying AVR's `utoa` via a force-included `emulator/host_shim.h`. It cannot reproduce hardware flicker (`RefreshAll` is a host no-op).

Note: in some sandboxed environments the AddressSanitizer runtime hangs at startup; the suites are designed to also pass under UBSan / plain builds, which carry the same assertions.

## Architecture

The engine is **decoupled from the display hardware** behind a zero-cost grid abstraction. This is the key thing to understand before editing:

- **`ConwayGrid`** (`ConwayGrid.h`) is a compile-time typedef (no virtual dispatch) selecting a backend by build flag:
  - `Peggy2Grid` (`Peggy2Grid.h`) — inlined wrapper over a real `Peggy2` framebuffer (on-device).
  - `HostGrid` (`host/HostGrid.h`) — in-memory bitmap with no Arduino dependency (host builds, selected by `-DCONWAY_HOST_BUILD`).
  The engine only ever refers to `ConwayGrid`; it never includes `Peggy2Serial.h` directly. Grid dimensions live in `GridDimensions.h` (`ROWS`/`COLUMNS`, 25×25).
- **`ConwayEngine`** (`.h`/`.cpp`) — the simulation. Double-buffers two `ConwayGrid` frames (`currentGenIndex`/`nextGenIndex`): `ComputeNextGen()` writes the next state into the next frame while the current one is displayed, `CommitNextGen()` swaps them. Loop history is kept separately as a ring of compact 32-bit generation hashes (`loopHashes`, capacity `loopHistorySize`), not full frames. `ComputeNextGen()` records the current generation's hash (`hashFrame`, FNV-1a over the cells) before computing — so externally-seeded frames are captured too — and `DetectLoop()` reports a loop when the just-computed frame's hash matches any recent one.
- **`Peggy2GameOfLife.ino`** — orchestrator. A `SimpleTimer` fires `GameOfLifeStep()` every `STEP_PERIOD_MS` (500ms); `loop()` runs the timer and continuously refreshes the current frame. The sketch reaches Peggy-specific display helpers (`Line`, `PeggyWriter`) via `GetCurrentFrame()->Device()`.
- **`StepCounter`** (`.h`/`.cpp`) — current-run and all-time-max step counts. `counterToString` fills caller-owned fixed buffers (no per-call heap allocation).

### Conventions worth knowing before editing

- **Display refresh during compute.** The Peggy is a multiplexed/persistence-of-vision matrix — it must be refreshed continuously or it goes dark. The per-step timer callback runs synchronously inside `loop()`, so `ComputeNextGen()` refreshes the *current* (displayed) frame once per row while computing the *next* one. Don't remove this thinking `loop()` covers it — it doesn't, because `loop()` is paused during the callback. (This is why `RefreshAll` is part of the `ConwayGrid` interface; it's a no-op on `HostGrid`.)
- **Double buffering.** Compute writes into `nextGen` while `currentGen` is displayed, so the shown image never tears during a step.
- **Toroidal wraparound.** `getCurrentCell` wraps edges with explicit `(i + N) % N` modulo over `ROWS`/`COLUMNS`, using signed coordinates — correct on both the 16-bit-`int` AVR and a 32-bit-`int` host. (The original `== -1` test only worked by accident of 16-bit integer promotion.) Making the wrap optional (toroidal vs. dead border) is still open work.
- **Loop detection is hash-based and bounded by `loopHistorySize`.** Detection stores one 32-bit hash per generation (not full frames), so a deep window is cheap: the sketch constructs the engine with `LOOP_HISTORY` (32) and the host emulator with 128, catching far longer oscillator periods than the original 4-frame ring — e.g. a glider's 100-generation torus recurrence. The trade-off is a ~2⁻³² chance a hash collision reports a loop one generation early (a harmless slightly-early reseed). It still won't catch periods longer than the window; a max-generation watchdog (issue #8) remains the backstop for those. Population-stagnation detection (the second half of issue #9) is still open.
- **The step/display state machine** lives in `GameOfLifeStep()` via the `displayStepCounter` flag: a normal step computes → checks for a loop → either commits or flips the flag; the next tick shows the counter screen (`ShowCounterScreen`), reseeds (`SeedRandom()` → `srand(analogRead(A0))`), and restarts.
- **Conway rule** is in `isAlive()`: survives on 2 neighbors (keeps state), born on 3, dead otherwise.

### Init patterns

`InitFrame` enum selects the seed: `Random` (production), `Blinker`, `RPentomino`, `Glider`. The non-random ones are fixed coordinate sets used as test fixtures and for debugging specific behaviors — switch the `Initialize(...)` argument to use them.

## Working in this repo

The fixes and refactor are organized as a **stack of PRs** (host harness → edge-wrap → tests → cleanup → leak → RNG), with follow-up PRs addressing review comments based on the branch they correct. When changing the engine, prefer host tests to validate logic; the firmware itself and any display/refresh (flicker) behavior can only be confirmed on the actual board.
