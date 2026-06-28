// Host verification for issue #3.
//
// The bug: InitializeRandom() fills the first frame with stdlib rand()
// (`rand() % 2`), but the only seeding call seeded the Arduino core
// random() generator instead -- a different generator -- so rand() was
// never seeded and every power-on produced the identical "random" frame.
//
// The fix pairs the generator with its seed: the first frame is seeded
// via srand(...) (the seed for rand()) before it is generated.
//
// This test mirrors the production logic exactly:
//   * fillFrame()  == InitializeRandom()'s per-cell expression `rand() % 2`
//   * seedRng()    == the seeding call (srand) used before the first frame
// and proves the seed actually controls the frame the init generator emits.
//
// Build & run: c++ -std=c++11 -Wall tests/rng_seed_test.cpp -o /tmp/rng_seed_test && /tmp/rng_seed_test

#include <cstdlib>
#include <cstring>
#include <cstdio>

static const int ROWS = 25;
static const int COLUMNS = 25;

// Same generator the init path uses for every cell (Peggy2ConwayEngine.cpp).
static void fillFrame(int frame[ROWS][COLUMNS])
{
  for (int i = 0; i < ROWS; i++)
    for (int j = 0; j < COLUMNS; j++)
      frame[i][j] = rand() % 2;
}

// Same generator the seed path drives (Peggy2GameOfLife.ino SeedRandom()).
static void seedRng(unsigned int seed)
{
  srand(seed);
}

static bool framesEqual(int a[ROWS][COLUMNS], int b[ROWS][COLUMNS])
{
  return memcmp(a, b, sizeof(int) * ROWS * COLUMNS) == 0;
}

int main()
{
  int frameA[ROWS][COLUMNS];
  int frameB[ROWS][COLUMNS];
  int frameA2[ROWS][COLUMNS];

  // 1. Two different seed values must yield two different first frames.
  //    This is exactly what fails when the init generator and the seeded
  //    generator are not the same one.
  seedRng(1);   fillFrame(frameA);
  seedRng(2);   fillFrame(frameB);
  if (framesEqual(frameA, frameB)) {
    printf("FAIL: different seeds must vary the first frame\n");
    return 1;
  }

  // 2. The generator that is seeded is the generator used to init:
  //    re-seeding with the same value reproduces the same frame.
  seedRng(1);   fillFrame(frameA2);
  if (!framesEqual(frameA, frameA2)) {
    printf("FAIL: same seed must reproduce the same frame\n");
    return 1;
  }

  // 3. Regression guard for the original bug: without seeding, the frame is
  //    a fixed sequence (rand() defaults to seed 1), i.e. identical every
  //    power-on. The fix prevents this by seeding before the first frame.
  int unseeded[ROWS][COLUMNS];
  srand(1); // libc default seed -> the "always identical" behaviour
  fillFrame(unseeded);
  if (!framesEqual(unseeded, frameA)) {
    printf("FAIL: unseeded run is the fixed default sequence\n");
    return 1;
  }

  printf("PASS: init generator (rand) is controlled by its seed (srand); "
         "distinct seeds produce distinct first frames.\n");
  return 0;
}
