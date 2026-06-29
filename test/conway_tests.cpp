// Host unit tests for ConwayEngine against canonical Game of Life
// patterns (issue #6). Built off-device against the shared host harness
// (HostGrid, selected via -DCONWAY_HOST_BUILD; issue #5).
//
// All fixtures live in the interior of the 25x25 grid, away from the edges, so
// the engine's toroidal wrapping does not affect the expected results and these
// tests pin standard infinite-grid Conway behavior.

#include "ConwayEngine.h"

#include <cstdio>
#include <vector>
#include <utility>

using Cell = std::pair<int, int>;   // (x = column, y = row)
using Cells = std::vector<Cell>;

// Shared interior fixtures (kept off the edges so toroidal wrapping doesn't
// affect the expected results).
static const Cells kBlock  = {{11, 11}, {12, 11}, {11, 12}, {12, 12}};
static const Cells kGlider = {{10, 10}, {12, 10}, {12, 11}, {11, 11}, {11, 12}};

// ---- tiny test harness -----------------------------------------------------
static int g_checks = 0;
static int g_failures = 0;

static void check(bool ok, const char* what)
{
  g_checks++;
  if (!ok)
  {
    g_failures++;
    printf("    FAIL: %s\n", what);
  }
}

// ---- frame helpers ---------------------------------------------------------
static int population(ConwayGrid* f)
{
  int n = 0;
  for (int y = 0; y < ROWS; y++)
    for (int x = 0; x < COLUMNS; x++)
      n += f->GetPoint(x, y) ? 1 : 0;
  return n;
}

static void setCells(ConwayGrid* f, const Cells& cells)
{
  f->Clear();
  for (const Cell& c : cells) f->WritePoint(c.first, c.second, 1);
}

// population matches and every listed cell is alive (and nothing else)
static bool hasExactly(ConwayGrid* f, const Cells& cells)
{
  if (population(f) != (int)cells.size()) return false;
  for (const Cell& c : cells)
    if (!f->GetPoint(c.first, c.second)) return false;
  return true;
}

static bool sameFrame(ConwayGrid* a, ConwayGrid* b)
{
  return a->Equals(*b);
}

// advance the engine by one committed generation
static void advance(ConwayEngine& e)
{
  e.ComputeNextGen();
  e.CommitNextGen();
}

// ---- tests -----------------------------------------------------------------

// Blinker oscillates with period 2: horizontal -> vertical -> horizontal.
static void test_blinker_period2()
{
  printf("test_blinker_period2\n");
  ConwayEngine e(4);
  e.Initialize(Blinker);  // horizontal bar at (12..14, 11)

  ConwayGrid gen0 = *e.GetCurrentFrame();
  check(population(e.GetCurrentFrame()) == 3, "blinker gen0 population is 3");

  advance(e);
  check(population(e.GetCurrentFrame()) == 3, "blinker gen1 population is 3");
  check(!sameFrame(e.GetCurrentFrame(), &gen0), "blinker gen1 differs from gen0");

  advance(e);
  check(sameFrame(e.GetCurrentFrame(), &gen0), "blinker returns to gen0 after period 2");
}

// A 2x2 block is a still life: unchanged generation after generation.
static void test_block_still_life()
{
  printf("test_block_still_life\n");
  ConwayEngine e(4);
  e.Initialize(Blinker);          // seeds indices and clears all slots
  setCells(e.GetCurrentFrame(), kBlock);

  for (int gen = 1; gen <= 3; gen++)
  {
    advance(e);
    check(hasExactly(e.GetCurrentFrame(), kBlock), "block unchanged after a generation");
  }
}

// A glider returns to its shape after 4 generations, shifted by (1, 1).
static void test_glider_translation()
{
  printf("test_glider_translation\n");
  const Cells expected = {{11, 11}, {13, 11}, {13, 12}, {12, 12}, {12, 13}};

  ConwayEngine e(5);
  e.Initialize(Blinker);
  setCells(e.GetCurrentFrame(), kGlider);

  for (int gen = 0; gen < 4; gen++) advance(e);

  check(hasExactly(e.GetCurrentFrame(), expected),
        "glider reproduced shifted by (1,1) after 4 generations");
}

// R-pentomino reaches known populations at known generations.
static void test_rpentomino_population()
{
  printf("test_rpentomino_population\n");
  // Independently verified (infinite-grid Conway): 5,6,7,9,8,9,12
  const int expectedPop[] = {5, 6, 7, 9, 8, 9, 12};

  ConwayEngine e(4);
  e.Initialize(RPentomino);

  check(population(e.GetCurrentFrame()) == expectedPop[0], "R-pentomino gen0 population is 5");
  for (int gen = 1; gen <= 6; gen++)
  {
    advance(e);
    char msg[64];
    snprintf(msg, sizeof(msg), "R-pentomino gen%d population is %d", gen, expectedPop[gen]);
    check(population(e.GetCurrentFrame()) == expectedPop[gen], msg);
  }
}

// DetectLoop fires for a still life (matches the immediately preceding frame).
static void test_detectloop_still_life()
{
  printf("test_detectloop_still_life\n");
  ConwayEngine e(4);
  e.Initialize(Blinker);
  setCells(e.GetCurrentFrame(), kBlock);

  e.ComputeNextGen();
  check(e.DetectLoop(), "DetectLoop fires for a still life");
}

// DetectLoop fires for a short oscillator once the state recurs in memory.
static void test_detectloop_oscillator()
{
  printf("test_detectloop_oscillator\n");
  ConwayEngine e(4);
  e.Initialize(Blinker);

  // Step 1: next gen is the vertical phase, not yet seen -> no loop.
  e.ComputeNextGen();
  check(!e.DetectLoop(), "DetectLoop quiet on first distinct generation");
  e.CommitNextGen();

  // Step 2: next gen returns to the horizontal phase stored in memory -> loop.
  e.ComputeNextGen();
  check(e.DetectLoop(), "DetectLoop fires for a period-2 oscillator");
}

// DetectLoop stays quiet while a glider keeps translating (never recurs).
static void test_detectloop_evolving()
{
  printf("test_detectloop_evolving\n");
  ConwayEngine e(4);
  e.Initialize(Blinker);
  setCells(e.GetCurrentFrame(), kGlider);

  for (int gen = 0; gen < 6; gen++)
  {
    e.ComputeNextGen();
    check(!e.DetectLoop(), "DetectLoop quiet for an evolving (translating) pattern");
    e.CommitNextGen();
  }
}

// Steps until DetectLoop fires (returns the generation at which it fired) or
// returns -1 if it never fired within maxGen generations.
static int stepUntilLoop(ConwayEngine& e, int maxGen)
{
  for (int gen = 1; gen <= maxGen; gen++)
  {
    e.ComputeNextGen();
    if (e.DetectLoop()) return gen;
    e.CommitNextGen();
  }
  return -1;
}

// A glider translates by (1,1) every 4 generations, so on the 25x25 torus it
// returns to its exact starting configuration after 25*4 = 100 generations.
// With a loop-detection history deeper than that period, DetectLoop catches the
// recurrence -- a far longer period than the old 4-frame ring could ever see.
static void test_detectloop_long_period_deep_history()
{
  printf("test_detectloop_long_period_deep_history\n");
  ConwayEngine e(128);            // history deeper than the glider's torus period
  e.Initialize(Blinker);
  setCells(e.GetCurrentFrame(), kGlider);

  int firedAt = stepUntilLoop(e, 150);
  check(firedAt == 100, "deep history catches the glider's period-100 torus recurrence");
}

// The same glider with a shallow history (like the original 4-frame ring) never
// recurs within the window, so DetectLoop must stay quiet: a watchdog, not loop
// detection, is what eventually reseeds such long-period patterns.
static void test_detectloop_long_period_shallow_misses()
{
  printf("test_detectloop_long_period_shallow_misses\n");
  ConwayEngine e(4);
  e.Initialize(Blinker);
  setCells(e.GetCurrentFrame(), kGlider);

  int firedAt = stepUntilLoop(e, 150);
  check(firedAt == -1, "shallow history never falsely fires on a long-period glider");
}

// A zero-length history disables loop detection: even a still life (which would
// otherwise be caught immediately) must never trigger, with no UB from the
// modulo-by-size in the history bookkeeping.
static void test_detectloop_disabled_with_zero_history()
{
  printf("test_detectloop_disabled_with_zero_history\n");
  ConwayEngine e(0);
  e.Initialize(Blinker);
  setCells(e.GetCurrentFrame(), kBlock);

  for (int gen = 0; gen < 5; gen++)
  {
    e.ComputeNextGen();
    check(!e.DetectLoop(), "DetectLoop disabled when history size is 0");
    e.CommitNextGen();
  }
}

// ---- max-generation watchdog (issue #8) ------------------------------------

// Steps mirroring the sketch's state machine; returns the generation at which
// WatchdogExpired() first fires, or -1 if it never fired within maxSteps. `gen`
// tracks the engine's committed-generation count: both start at 1 and increment
// in lockstep (loop `gen++` mirrors CommitNextGen), so a fire at `gen == N`
// means the engine had N committed generations.
static int stepUntilWatchdog(ConwayEngine& e, int maxSteps)
{
  for (int gen = 1; gen <= maxSteps; gen++)
  {
    e.ComputeNextGen();
    if (e.WatchdogExpired()) return gen;
    e.CommitNextGen();
  }
  return -1;
}

// With loop detection off, a glider keeps translating forever; the watchdog is
// the only thing that stops it, and it must fire exactly at the configured cap.
static void test_watchdog_fires_at_cap()
{
  printf("test_watchdog_fires_at_cap\n");
  ConwayEngine e(EngineConfig{0, 5, 0});   // loop off, cap 5, stagnation off
  e.Initialize(Blinker);
  setCells(e.GetCurrentFrame(), kGlider);

  check(stepUntilWatchdog(e, 50) == 5, "watchdog fires at the configured generation cap");
}

// A zero cap disables the watchdog entirely: an endlessly translating glider
// must never trip it.
static void test_watchdog_disabled_when_zero()
{
  printf("test_watchdog_disabled_when_zero\n");
  ConwayEngine e(EngineConfig{0, 0, 0});   // everything off
  e.Initialize(Blinker);
  setCells(e.GetCurrentFrame(), kGlider);

  check(stepUntilWatchdog(e, 200) == -1, "watchdog never fires when the cap is 0");
}

// Re-Initialize starts a fresh run: the generation counter resets, so the
// watchdog fires at the cap again rather than immediately.
static void test_watchdog_resets_on_initialize()
{
  printf("test_watchdog_resets_on_initialize\n");
  ConwayEngine e(EngineConfig{0, 5, 0});
  e.Initialize(Blinker);
  setCells(e.GetCurrentFrame(), kGlider);
  check(stepUntilWatchdog(e, 50) == 5, "watchdog fires on the first run");

  e.Initialize(Blinker);
  setCells(e.GetCurrentFrame(), kGlider);
  check(stepUntilWatchdog(e, 50) == 5, "watchdog fires again at the cap after re-init");
}

// ---- population-stagnation detection (issue #9 tail) -----------------------

// Steps mirroring the state machine; returns the generation at which
// PopulationStagnant() first fires, or -1 within maxSteps.
static int stepUntilStagnant(ConwayEngine& e, int maxSteps)
{
  for (int gen = 1; gen <= maxSteps; gen++)
  {
    e.ComputeNextGen();
    if (e.PopulationStagnant()) return gen;
    e.CommitNextGen();
  }
  return -1;
}

// A block is a still life: its live-cell count never changes, so with a window
// of N the stagnation heuristic fires once N consecutive generations have shared
// the same population.
static void test_population_stagnation_fires_on_constant_population()
{
  printf("test_population_stagnation_fires_on_constant_population\n");
  ConwayEngine e(EngineConfig{0, 0, 4});   // loop/watchdog off, window 4
  e.Initialize(Blinker);
  setCells(e.GetCurrentFrame(), kBlock);

  check(stepUntilStagnant(e, 50) == 4, "stagnation fires after a full window of unchanged population");
}

// The R-pentomino's population changes every early generation (5,6,7,9,8,9,...),
// so a short window must never trip while the population keeps moving.
static void test_population_stagnation_quiet_while_population_changes()
{
  printf("test_population_stagnation_quiet_while_population_changes\n");
  ConwayEngine e(EngineConfig{0, 0, 3});   // window 3
  e.Initialize(RPentomino);

  // Only step through the early, population-changing generations.
  check(stepUntilStagnant(e, 6) == -1, "stagnation quiet while population keeps changing");
}

// A zero window disables stagnation detection: even a still life never trips it.
static void test_population_stagnation_disabled_when_zero()
{
  printf("test_population_stagnation_disabled_when_zero\n");
  ConwayEngine e(EngineConfig{0, 0, 0});
  e.Initialize(Blinker);
  setCells(e.GetCurrentFrame(), kBlock);

  check(stepUntilStagnant(e, 50) == -1, "stagnation never fires when the window is 0");
}

// Re-Initialize clears the stagnation run, so a fresh constant-population run
// fires at the window again rather than carrying over.
static void test_population_stagnation_resets_on_initialize()
{
  printf("test_population_stagnation_resets_on_initialize\n");
  ConwayEngine e(EngineConfig{0, 0, 4});
  e.Initialize(Blinker);
  setCells(e.GetCurrentFrame(), kBlock);
  check(stepUntilStagnant(e, 50) == 4, "stagnation fires on the first run");

  e.Initialize(Blinker);
  setCells(e.GetCurrentFrame(), kBlock);
  check(stepUntilStagnant(e, 50) == 4, "stagnation fires again at the window after re-init");
}

// The single-argument convenience constructor configures loop detection only:
// the watchdog and stagnation detector stay disabled, so an evolving glider runs
// indefinitely without either backstop firing.
static void test_convenience_ctor_leaves_backstops_disabled()
{
  printf("test_convenience_ctor_leaves_backstops_disabled\n");
  ConwayEngine e(4);
  e.Initialize(Blinker);
  setCells(e.GetCurrentFrame(), kGlider);

  for (int gen = 0; gen < 50; gen++)
  {
    e.ComputeNextGen();
    check(!e.WatchdogExpired(), "convenience ctor leaves the watchdog disabled");
    check(!e.PopulationStagnant(), "convenience ctor leaves stagnation disabled");
    e.CommitNextGen();
  }
}

int main()
{
  test_blinker_period2();
  test_block_still_life();
  test_glider_translation();
  test_rpentomino_population();
  test_detectloop_still_life();
  test_detectloop_oscillator();
  test_detectloop_evolving();
  test_detectloop_long_period_deep_history();
  test_detectloop_long_period_shallow_misses();
  test_detectloop_disabled_with_zero_history();
  test_watchdog_fires_at_cap();
  test_watchdog_disabled_when_zero();
  test_watchdog_resets_on_initialize();
  test_population_stagnation_fires_on_constant_population();
  test_population_stagnation_quiet_while_population_changes();
  test_population_stagnation_disabled_when_zero();
  test_population_stagnation_resets_on_initialize();
  test_convenience_ctor_leaves_backstops_disabled();

  printf("\n%d checks, %d failures\n", g_checks, g_failures);
  if (g_failures == 0) printf("ALL TESTS PASSED\n");
  return g_failures == 0 ? 0 : 1;
}
