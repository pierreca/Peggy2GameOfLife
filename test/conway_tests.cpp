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

int main()
{
  test_blinker_period2();
  test_block_still_life();
  test_glider_translation();
  test_rpentomino_population();
  test_detectloop_still_life();
  test_detectloop_oscillator();
  test_detectloop_evolving();

  printf("\n%d checks, %d failures\n", g_checks, g_failures);
  if (g_failures == 0) printf("ALL TESTS PASSED\n");
  return g_failures == 0 ? 0 : 1;
}
