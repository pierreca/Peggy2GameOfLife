// Host test for toroidal edge-wrap neighbour counting (issue #4).
//
// On the AVR target `int` is 16-bit, so the original `unsigned short`
// coordinates with `if (x == -1)` wrapped by accident of integer promotion.
// On a host (32-bit int) that wrap never fires and getCurrentCell reads out of
// bounds. This test pins the *toroidal* behaviour on the host: a single live
// cell must contribute to the neighbour counts of cells on the opposite edges.
//
// We compile against the host build of the engine (CONWAY_HOST_BUILD selects the
// in-memory HostGrid from the shared test harness, issue #5). `#define private
// public` lets us call the private grid/wrap helpers directly without modifying
// production code.
//
// Build/run (see run.sh; also how it FAILS on the original code, with ASan):
//   clang++ -std=c++14 -fsanitize=address -g -DCONWAY_HOST_BUILD -I. -Ihost \
//       test/test_edge_wrap.cpp Peggy2ConwayEngine.cpp -o /tmp/test_edge_wrap
//   /tmp/test_edge_wrap

#define private public
#include "../Peggy2ConwayEngine.h"
#undef private

#include <cstdio>
#include <cstdlib>

static int g_failures = 0;

static void check(const char* label, int got, int expected)
{
  if (got != expected)
  {
    std::printf("FAIL: %-34s got %d, expected %d\n", label, got, expected);
    g_failures++;
  }
  else
  {
    std::printf("ok:   %-34s = %d\n", label, got);
  }
}

int main()
{
  // genMemorySize of 2 (current + next) is the minimum the engine needs.
  Peggy2ConwayEngine engine(2);
  engine.Initialize(Glider);          // sets indices; we overwrite the grid below

  ConwayGrid* grid = engine.GetCurrentFrame();
  grid->Clear();

  // ---- Single live cell in the far corner (24,24). --------------------------
  // Toroidally it is a neighbour of the opposite-corner / opposite-edge cells,
  // reached only through the x-1 / y-1 ("== -1") wrap that breaks on the host.
  grid->WritePoint(24, 24, 1);

  check("neighbors(0,0)   wrap -1,-1",  engine.getNeighborCount(0, 0),  1);
  check("neighbors(0,24)  wrap -1, 0",  engine.getNeighborCount(0, 24), 1);
  check("neighbors(24,0)  wrap  0,-1",  engine.getNeighborCount(24, 0), 1);
  check("neighbors(23,23) no wrap",     engine.getNeighborCount(23, 23), 1);
  check("neighbors(12,12) far cell",    engine.getNeighborCount(12, 12), 0);
  check("self (24,24) excludes self",   engine.getNeighborCount(24, 24), 0);

  // ---- Single live cell at origin (0,0). ------------------------------------
  // Symmetric check exercising the x+1==25 / y+1==25 wrap from the origin and
  // confirming the opposite corner sees it.
  grid->Clear();
  grid->WritePoint(0, 0, 1);

  check("neighbors(24,24) wrap +1,+1",  engine.getNeighborCount(24, 24), 1);
  check("neighbors(24,0)  wrap +1, 0",  engine.getNeighborCount(24, 0), 1);
  check("neighbors(0,24)  wrap  0,+1",  engine.getNeighborCount(0, 24), 1);
  check("neighbors(1,1)   no wrap",     engine.getNeighborCount(1, 1),  1);

  if (g_failures == 0)
  {
    std::printf("\nALL PASS\n");
    return 0;
  }
  std::printf("\n%d FAILURE(S)\n", g_failures);
  return 1;
}
