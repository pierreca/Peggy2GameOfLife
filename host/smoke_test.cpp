// Host smoke test for the Conway engine.
//
// This compiles the real ConwayEngine against the in-memory HostGrid
// (no Arduino toolchain, no Peggy2Serial) and checks that the engine actually
// advances a generation. The classic test is a "blinker": a horizontal row of
// three live cells that must become a vertical column of three after one step.
//
// Build & run:  make -C host test   (or: cd host && make test)

#include <cstdio>
#include "ConwayEngine.h"

static int failures = 0;

static void check(bool condition, const char* message)
{
  if (!condition)
  {
    failures++;
    printf("  FAIL: %s\n", message);
  }
}

int main()
{
  printf("Conway engine host smoke test\n");

  // Loop-detection history depth (unused by this single-step smoke test).
  ConwayEngine engine(4);
  engine.Initialize(Blinker);

  // The blinker seed: three horizontal cells at row 11, columns 12..14.
  ConwayGrid* seed = engine.GetCurrentFrame();
  check(seed->GetPoint(12, 11) == 1, "seed cell (12,11) alive");
  check(seed->GetPoint(13, 11) == 1, "seed cell (13,11) alive");
  check(seed->GetPoint(14, 11) == 1, "seed cell (14,11) alive");

  // Advance one generation.
  engine.ComputeNextGen();
  engine.CommitNextGen();

  // After one step a horizontal blinker becomes vertical: column 13, rows 10..12.
  ConwayGrid* next = engine.GetCurrentFrame();
  check(next->GetPoint(13, 10) == 1, "stepped cell (13,10) alive");
  check(next->GetPoint(13, 11) == 1, "stepped cell (13,11) alive");
  check(next->GetPoint(13, 12) == 1, "stepped cell (13,12) alive");
  check(next->GetPoint(12, 11) == 0, "former cell (12,11) now dead");
  check(next->GetPoint(14, 11) == 0, "former cell (14,11) now dead");

  // The grid must have actually changed (the engine advanced).
  check(!seed->Equals(*next), "generation advanced (frames differ)");

  if (failures == 0)
  {
    printf("PASS: engine seeded a blinker and advanced it correctly\n");
    return 0;
  }

  printf("FAILED: %d check(s) failed\n", failures);
  return 1;
}
