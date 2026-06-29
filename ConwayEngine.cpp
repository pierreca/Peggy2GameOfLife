#include <stdlib.h>
#include "ConwayEngine.h"

ConwayEngine::ConwayEngine(const EngineConfig& config)
{
  this->loopHistorySize = config.loopHistorySize;
  this->loopHashes = new uint32_t[loopHistorySize]; // new[0] is valid and freeable
  this->loopHashCount = 0;
  this->loopHashHead = 0;

  this->maxGenerations = config.maxGenerations;
  this->generationCount = 0;

  this->populationWindow = config.populationWindow;
  this->lastPopulation = -1;
  this->stagnantRun = 0;

  this->genMemory = new ConwayGrid*[frameCount];
  for (unsigned short i = 0; i < frameCount; i++)
  {
    this->genMemory[i] = new ConwayGrid();
  }
}

// Convenience: loop detection only, both backstops disabled.
ConwayEngine::ConwayEngine(unsigned short loopHistorySize)
  : ConwayEngine(EngineConfig{loopHistorySize, 0, 0})
{
}

ConwayEngine::~ConwayEngine()
{
  for (unsigned short i = 0; i < frameCount; i++)
  {
    delete this->genMemory[i];
  }
  delete[] this->genMemory;
  delete[] this->loopHashes;
}

void ConwayEngine::Initialize(InitFrame initializationType)
{
  this->currentGenIndex = 0;
  this->nextGenIndex = 1;

  // A fresh run shares no loop-detection history with the previous one.
  this->loopHashCount = 0;
  this->loopHashHead = 0;

  // Reset both backstops: the seeded frame is generation 1, and no population
  // has been recorded yet for this run.
  this->generationCount = 1;
  this->lastPopulation = -1;
  this->stagnantRun = 0;

  this->genMemory[this->currentGenIndex]->HardwareInit();
  this->genMemory[this->nextGenIndex]->Clear();

  switch(initializationType)
  {
    case Random:
      this->InitializeRandom();
      break;
    case Blinker:
      this->InitializeBlinker();
      break;
    case RPentomino:
      this->InitializeRPentomino();
      break;
    case Glider:
      this->InitializeGlider();
      break;
    default:  
      this->InitializeRandom();
      break;
  }
}

ConwayGrid* ConwayEngine::GetCurrentFrame()
{
  return this->genMemory[this->currentGenIndex];
}

void ConwayEngine::ComputeNextGen()
{
  ConwayGrid* currentGen = this->genMemory[this->currentGenIndex];
  ConwayGrid* nextGen = this->genMemory[this->nextGenIndex];

  // Remember the current generation so DetectLoop can later recognise a return
  // to it (or to any recent generation). Recorded here, right before computing
  // the next frame, so a frame seeded directly into the current buffer (e.g. by
  // a test or a non-random Initialize) is captured too.
  this->recordHash(this->hashFrame(currentGen));

  // Track the current generation's population for stagnation detection, on the
  // same frame and at the same point as the hash so both series stay aligned.
  // Skipped when disabled to avoid scanning the whole frame for nothing.
  if (this->populationWindow > 0)
  {
    this->recordPopulation(this->countPopulation(currentGen));
  }

  for (int i = 0; i < ROWS; i++)
  {
    for (int j = 0; j < COLUMNS; j++)
    {
      nextGen->WritePoint(j, i, isAlive(currentGen->GetPoint(j, i), getNeighborCount(j, i)));
    }
    // Keep the displayed (current) frame lit during this blocking compute pass.
    // The Peggy 2 is a multiplexed matrix that goes dark without a continuous
    // RefreshAll, and the timer callback runs synchronously inside loop(), so
    // loop()'s own refresh is paused while we compute. Refreshing once per row
    // (vs once per cell originally) keeps persistence-of-vision flicker-free at
    // a fraction of the overhead. RefreshAll is part of the ConwayGrid interface
    // (a no-op on the host grid), so this stays display-agnostic.
    currentGen->RefreshAll(1);
  }
}

void ConwayEngine::CommitNextGen()
{
  this->currentGenIndex = (this->currentGenIndex + 1) % frameCount;
  this->nextGenIndex = (this->nextGenIndex + 1) % frameCount;
  this->generationCount++;
}

bool ConwayEngine::DetectLoop()
{
  // A zero-length history disables loop detection entirely.
  if (this->loopHistorySize == 0)
  {
    return false;
  }

  // The just-computed next generation loops the simulation if its state matches
  // any recent generation. Compare hashes rather than full frames so the history
  // can stay deep cheaply (see the header note on the collision trade-off).
  uint32_t candidate = this->hashFrame(this->genMemory[this->nextGenIndex]);

  for (unsigned short i = 0; i < this->loopHashCount; i++)
  {
    if (this->loopHashes[i] == candidate)
    {
      return true;
    }
  }

  return false;
}

// Max-generation watchdog: a run that has been displayed for maxGenerations
// committed generations is forced to reseed, regardless of loop detection. This
// is the backstop for gliders/spaceships and long-period or chaotic fields that
// never recur within the loop-history window. A 0 cap disables it.
bool ConwayEngine::WatchdogExpired()
{
  return this->maxGenerations > 0 && this->generationCount >= this->maxGenerations;
}

// Population stagnation: when the live-cell count has not changed for
// populationWindow consecutive generations the field is treated as settled or
// stuck and forced to reseed. Cheaper and complementary to loop detection (no
// exact state match needed) and orthogonal to the watchdog. A 0 window disables
// it. (recordPopulation only runs while enabled, so stagnantRun is meaningless
// otherwise; the window guard here keeps that case quiet.)
bool ConwayEngine::PopulationStagnant()
{
  return this->populationWindow > 0 && this->stagnantRun >= this->populationWindow;
}

// FNV-1a over the frame's cells, packed one row at a time. COLUMNS <= 32 so each
// row fits in the 32-bit accumulator; the result is identical on the 16-bit-int
// AVR and a 32-bit-int host because uint32_t arithmetic is fixed width.
uint32_t ConwayEngine::hashFrame(ConwayGrid* frame)
{
  uint32_t hash = 2166136261u; // FNV-1a offset basis
  for (int y = 0; y < ROWS; y++)
  {
    uint32_t row = 0;
    for (int x = 0; x < COLUMNS; x++)
    {
      row = (row << 1) | (frame->GetPoint(x, y) ? 1u : 0u);
    }
    for (int b = 0; b < 4; b++)
    {
      hash ^= (row >> (b * 8)) & 0xFFu;
      hash *= 16777619u; // FNV-1a prime
    }
  }
  return hash;
}

// Push a generation hash into the circular history, dropping the oldest once the
// window is full.
void ConwayEngine::recordHash(uint32_t hash)
{
  // Nothing to record (and no modulo by zero) when detection is disabled.
  if (this->loopHistorySize == 0)
  {
    return;
  }

  this->loopHashes[this->loopHashHead] = hash;
  this->loopHashHead = (this->loopHashHead + 1) % this->loopHistorySize;
  if (this->loopHashCount < this->loopHistorySize)
  {
    this->loopHashCount++;
  }
}

// Count the live cells in a frame (for stagnation detection).
unsigned short ConwayEngine::countPopulation(ConwayGrid* frame)
{
  unsigned short count = 0;
  for (int y = 0; y < ROWS; y++)
  {
    for (int x = 0; x < COLUMNS; x++)
    {
      if (frame->GetPoint(x, y)) count++;
    }
  }
  return count;
}

// Extend or restart the run of consecutive equal-population generations. The
// counter saturates instead of wrapping so a population that stays constant far
// past the window (e.g. with stagnation detection enabled but its result
// ignored) can never silently roll back under the window.
void ConwayEngine::recordPopulation(unsigned short population)
{
  if (this->lastPopulation == (int)population)
  {
    if (this->stagnantRun < 0xFFFF) this->stagnantRun++;
  }
  else
  {
    this->lastPopulation = (int)population;
    this->stagnantRun = 1;
  }
}

/* Known Initialization sequences */
void ConwayEngine::InitializeRandom()
{
  ConwayGrid* currentGen = this->genMemory[this->currentGenIndex];
  
  for (int i = 0; i < ROWS; i++)
  {
    for(int j = 0; j < COLUMNS; j++)
    {
      int ledState = rand() % 2;
      currentGen->WritePoint(j, i, ledState);
    }
  }
}

void ConwayEngine::InitializeGlider()
{
  ConwayGrid* currentGen = this->genMemory[this->currentGenIndex];
  currentGen->Clear();
  currentGen->WritePoint(0,0,1);
  currentGen->WritePoint(2,0,1);
  currentGen->WritePoint(2,1,1);
  currentGen->WritePoint(1,1,1);
  currentGen->WritePoint(1,2,1);
}

void ConwayEngine::InitializeRPentomino()
{
  ConwayGrid* currentGen = this->genMemory[this->currentGenIndex];
  currentGen->Clear();
  currentGen->WritePoint(12,11,1);
  currentGen->WritePoint(13,11,1);
  currentGen->WritePoint(11,12,1);  
  currentGen->WritePoint(12,12,1);
  currentGen->WritePoint(12,13,1);
}

void ConwayEngine::InitializeBlinker()
{
  ConwayGrid* currentGen = this->genMemory[this->currentGenIndex];
  currentGen->Clear();
  currentGen->WritePoint(12,11,1);
  currentGen->WritePoint(13,11,1);
  currentGen->WritePoint(14,11,1); 
}


unsigned short ConwayEngine::isAlive(unsigned short currentState, unsigned short neighborCount)
{
  unsigned short newState = 0;

  // error cases - should do something else than clipping...
  if (neighborCount > 8) neighborCount = 8;
  
  // if (neighborCount < 2 || neighborCount > 3) newState = 0; // useless, already initialized to false
  if (neighborCount == 2) newState = currentState;
  if (neighborCount == 3) newState = 1;

  return newState;
}

unsigned short ConwayEngine::getNeighborCount(int x, int y)
{
  unsigned short count = 0;

  count += this->getCurrentCell(x - 1, y - 1);
  count += this->getCurrentCell(x, y - 1);
  count += this->getCurrentCell(x + 1, y - 1);
  
  count += this->getCurrentCell(x - 1, y);
  count += this->getCurrentCell(x + 1, y);

  count += this->getCurrentCell(x - 1, y + 1);
  count += this->getCurrentCell(x, y + 1);
  count += this->getCurrentCell(x + 1, y + 1);
  
  return count;
}

unsigned short ConwayEngine::getCurrentCell(int x, int y)
{
  ConwayGrid* currentGen = this->genMemory[this->currentGenIndex];

  // Toroidal wrap with explicit, platform-independent modulo. Using signed
  // coordinates and (i + N) % N keeps the math correct on both the AVR
  // (16-bit int) and host compilers (32-bit int); the previous `== -1` test
  // only wrapped by accident of 16-bit integer promotion (see issue #4).
  x = (x + COLUMNS) % COLUMNS;
  y = (y + ROWS) % ROWS;

  return currentGen->GetPoint(x, y);
}
