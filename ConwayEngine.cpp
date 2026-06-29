#include <stdlib.h>
#include "ConwayEngine.h"

ConwayEngine::ConwayEngine(unsigned short loopHistorySize)
{
  this->loopHistorySize = loopHistorySize;
  this->loopHashes = new uint32_t[loopHistorySize];
  this->loopHashCount = 0;
  this->loopHashHead = 0;

  this->genMemory = new ConwayGrid*[frameCount];
  for (unsigned short i = 0; i < frameCount; i++)
  {
    this->genMemory[i] = new ConwayGrid();
  }
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
}

bool ConwayEngine::DetectLoop()
{
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
  this->loopHashes[this->loopHashHead] = hash;
  this->loopHashHead = (this->loopHashHead + 1) % this->loopHistorySize;
  if (this->loopHashCount < this->loopHistorySize)
  {
    this->loopHashCount++;
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
