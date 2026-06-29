// Terminal emulator / visualizer for the Conway engine (issue #27).
//
// Runs the REAL ConwayEngine and StepCounter against the in-memory HostGrid
// (no Arduino toolchain, no Peggy2 hardware) and renders the 25x25 grid to the
// terminal once per generation, so you can watch patterns evolve on a laptop.
//
// It mirrors the sketch's GameOfLifeStep() state machine
// (Peggy2GameOfLife.ino): seed -> step -> DetectLoop -> show the step-counter
// screen -> reseed -> repeat. The one thing it deliberately cannot reproduce is
// display flicker: that is a hardware multiplex / persistence-of-vision timing
// property, and on the host RefreshAll() is a no-op.
//
// Build & run:  make -C emulator run   (or see emulator/Makefile for flags)

#include <cstdio>
#include <cstdlib>
#include <cstring>
#include <csignal>
#include <ctime>
#include <chrono>
#include <thread>

#include "ConwayEngine.h"
#include "StepCounter.h"
#include "GridDimensions.h"

namespace
{
// ---- ANSI terminal helpers -------------------------------------------------
const char* const CLEAR_SCREEN = "\x1b[2J";
const char* const CURSOR_HOME  = "\x1b[H";
const char* const HIDE_CURSOR  = "\x1b[?25l";
const char* const SHOW_CURSOR  = "\x1b[?25h";

// Restore the cursor on the way out, however we exit (normal end, Ctrl-C).
void restoreTerminal()
{
  printf("%s", SHOW_CURSOR);
  fflush(stdout);
}

volatile std::sig_atomic_t g_stop = 0;
void onSignal(int) { g_stop = 1; }

// ---- Command-line options --------------------------------------------------
struct Options
{
  InitFrame pattern = Random;
  int delayMs = 120;          // pause between generations
  int counterPauseMs = 2000;  // how long the LAST/MAX screen lingers
  unsigned int seed = 0;      // 0 => derive from wall-clock time
  bool seedSet = false;
  bool once = false;          // stop after the first loop is detected
  long maxGen = 0;            // 0 => unbounded (matches the firmware)
};

bool parsePattern(const char* s, InitFrame& out)
{
  if (!strcmp(s, "random"))     { out = Random;     return true; }
  if (!strcmp(s, "blinker"))    { out = Blinker;    return true; }
  if (!strcmp(s, "glider"))     { out = Glider;     return true; }
  if (!strcmp(s, "rpentomino")) { out = RPentomino; return true; }
  return false;
}

const char* patternName(InitFrame p)
{
  switch (p)
  {
    case Blinker:    return "blinker";
    case RPentomino: return "rpentomino";
    case Glider:     return "glider";
    case Random:     default: return "random";
  }
}

void usage(const char* prog)
{
  printf("Usage: %s [options]\n", prog);
  printf("  --pattern <random|blinker|glider|rpentomino>  initial seed (default random)\n");
  printf("  --delay <ms>      pause between generations (default 120)\n");
  printf("  --seed <n>        RNG seed for a reproducible run (default: time-based)\n");
  printf("  --once            stop after the first detected loop instead of reseeding\n");
  printf("  --max-gen <n>     reseed after n generations even without a loop (default: unbounded)\n");
  printf("  --help            show this help\n");
}

bool parseArgs(int argc, char** argv, Options& o)
{
  for (int i = 1; i < argc; i++)
  {
    const char* a = argv[i];
    if (!strcmp(a, "--help") || !strcmp(a, "-h"))
    {
      usage(argv[0]);
      return false;
    }
    else if (!strcmp(a, "--once"))
    {
      o.once = true;
    }
    else if (!strcmp(a, "--pattern") && i + 1 < argc)
    {
      if (!parsePattern(argv[++i], o.pattern))
      {
        fprintf(stderr, "unknown pattern: %s\n", argv[i]);
        return false;
      }
    }
    else if (!strcmp(a, "--delay") && i + 1 < argc)
    {
      o.delayMs = atoi(argv[++i]);
    }
    else if (!strcmp(a, "--seed") && i + 1 < argc)
    {
      o.seed = (unsigned int)strtoul(argv[++i], nullptr, 10);
      o.seedSet = true;
    }
    else if (!strcmp(a, "--max-gen") && i + 1 < argc)
    {
      o.maxGen = atol(argv[++i]);
    }
    else
    {
      fprintf(stderr, "unknown or incomplete option: %s\n", a);
      usage(argv[0]);
      return false;
    }
  }
  return true;
}

// ---- Rendering -------------------------------------------------------------
// Draw the whole frame in one buffered write, redrawing from the cursor home
// position so the terminal updates in place instead of scrolling.
void renderGrid(ConwayGrid* grid, InitFrame pattern, unsigned int generation,
                const char* maxCount)
{
  static char out[ROWS * (COLUMNS * 2 + 8) + 256];
  int n = 0;
  // append() bounds every write against the buffer's remaining capacity.
  #define append(...) n += snprintf(out + n, sizeof(out) - n, __VA_ARGS__)

  append("%s", CURSOR_HOME);
  append(" Conway's Game of Life  -  pattern: %-10s\n", patternName(pattern));
  append(" generation: %-6u   all-time max: %-6s\n", generation, maxCount);

  // Top border.
  append(" +");
  for (int x = 0; x < COLUMNS; x++) append("--");
  append("+\n");

  for (int y = 0; y < ROWS; y++)
  {
    append(" |");
    for (int x = 0; x < COLUMNS; x++)
    {
      // Two cells wide so a live square reads roughly square in the terminal.
      append("%s", grid->GetPoint(x, y) ? "\xE2\x96\x88\xE2\x96\x88" : "  ");
    }
    append("|\n");
  }

  // Bottom border.
  append(" +");
  for (int x = 0; x < COLUMNS; x++) append("--");
  append("+\n");
  append(" Ctrl-C to quit.\n");
  #undef append

  fwrite(out, 1, n, stdout);
  fflush(stdout);
}

// The host stand-in for ShowCounterScreen(): the firmware draws LAST/MAX onto
// the Peggy with PeggyWriter, which has no host equivalent, so we print it.
void showCounterScreen(StepCounter& counter, const Options& o, bool looped)
{
  printf("%s%s", CURSOR_HOME, CLEAR_SCREEN);
  printf("\n\n   +--------------------------+\n");
  printf("   |  %-24s|\n", looped ? "LOOP DETECTED" : "MAX GEN REACHED");
  printf("   |                          |\n");
  printf("   |  LAST: %-6s            |\n", counter.GetCurrentCountString());
  printf("   |  MAX:  %-6s            |\n", counter.GetMaxCountString());
  printf("   +--------------------------+\n");
  fflush(stdout);
  std::this_thread::sleep_for(std::chrono::milliseconds(o.counterPauseMs));
  printf("%s", CLEAR_SCREEN);
}

void sleepMs(int ms)
{
  if (ms > 0) std::this_thread::sleep_for(std::chrono::milliseconds(ms));
}

} // namespace

int main(int argc, char** argv)
{
  Options opt;
  if (!parseArgs(argc, argv, opt))
  {
    return 1;
  }

  unsigned int seed = opt.seedSet ? opt.seed : (unsigned int)time(nullptr);
  srand(seed); // the same generator InitializeRandom() draws from
  printf("Seeding RNG with %u%s\n", seed, opt.seedSet ? "" : " (time-based)");

  std::signal(SIGINT, onSignal);
  std::signal(SIGTERM, onSignal);
  atexit(restoreTerminal);

  // 4 generations of loop-detection memory, matching setup() in the sketch.
  ConwayEngine engine(4);
  StepCounter stepCounter;

  engine.Initialize(opt.pattern);
  stepCounter.IncrementCount();

  printf("%s%s", HIDE_CURSOR, CLEAR_SCREEN);

  InitFrame currentPattern = opt.pattern;
  unsigned int generation = 1;
  while (!g_stop)
  {
    renderGrid(engine.GetCurrentFrame(), currentPattern, generation,
               stepCounter.GetMaxCountString());
    sleepMs(opt.delayMs);
    if (g_stop) break;

    engine.ComputeNextGen();

    bool looped = engine.DetectLoop();
    bool watchdog = opt.maxGen > 0 && (long)generation >= opt.maxGen;

    if (looped || watchdog)
    {
      // Commit so the counter screen / final frame reflects the latest step.
      engine.CommitNextGen();
      stepCounter.IncrementCount();
      renderGrid(engine.GetCurrentFrame(), currentPattern, generation,
                 stepCounter.GetMaxCountString());
      sleepMs(opt.delayMs);

      showCounterScreen(stepCounter, opt, looped);

      if (opt.once || g_stop)
      {
        break;
      }

      // Reseed and restart, exactly as the displayStepCounter branch does.
      stepCounter.ResetCurrentCount();
      engine.Initialize(Random); // the firmware always reseeds with Random
      stepCounter.IncrementCount();
      generation = 1;
    }
    else
    {
      engine.CommitNextGen();
      stepCounter.IncrementCount();
      generation++;
    }
  }

  printf("%sStopped.\n", SHOW_CURSOR);
  return 0;
}
