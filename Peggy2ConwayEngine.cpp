#include <stdlib.h>
#include <Peggy2Serial.h>
#include "Peggy2ConwayEngine.h"

Peggy2ConwayEngine::Peggy2ConwayEngine(unsigned short genMemorySize)
{
  this->genMemorySize = genMemorySize;
  this->genMemory = (Peggy2 **)malloc(genMemorySize * sizeof(Peggy2 *));
  for (unsigned short i = 0; i < genMemorySize; i++)
  {
    this->genMemory[i] = new Peggy2();
  }
}

void Peggy2ConwayEngine::Initialize(InitFrame initializationType)
{
  this->currentGenIndex = 0;
  this->nextGenIndex = 1;
  this->genMemory[this->currentGenIndex]->HardwareInit();
  
  for (int i = 1; i < this->genMemorySize; i++)
  {
    this->genMemory[i]->Clear();
  }
  
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

Peggy2* Peggy2ConwayEngine::GetCurrentFrame()
{
  return this->genMemory[this->currentGenIndex];
}

void Peggy2ConwayEngine::ComputeNextGen()
{
  Peggy2* currentGen = this->genMemory[this->currentGenIndex];
  Peggy2* nextGen = this->genMemory[this->nextGenIndex];
  
  for (int i = 0; i < ROWS; i++)
  {
    for (int j = 0; j < COLUMNS; j++)
    {
      nextGen->WritePoint(j, i, isAlive(currentGen->GetPoint(j, i), getNeighborCount(j, i)));
      currentGen->RefreshAll(1);
    }
  }
}

void Peggy2ConwayEngine::CommitNextGen() 
{
  this->currentGenIndex = (this->currentGenIndex + 1) % this->genMemorySize;
  this->nextGenIndex = (this->nextGenIndex + 1) % this->genMemorySize;
}

bool Peggy2ConwayEngine::DetectLoop()
{
  Peggy2* nextGen = this->genMemory[this->nextGenIndex];
  
  for (unsigned short i = 0; i < this->genMemorySize; i++)
  {
    if (i != this->nextGenIndex && this->areIdentical(nextGen, this->genMemory[i]))
    {
      return true;
    }
  }
  
  return false;
}

/* Known Initialization sequences */
void Peggy2ConwayEngine::InitializeRandom()
{
  Peggy2* currentGen = this->genMemory[this->currentGenIndex];
  
  for (int i = 0; i < ROWS; i++)
  {
    for(int j = 0; j < COLUMNS; j++)
    {
      int ledState = rand() % 2;
      currentGen->WritePoint(j, i, ledState);
    }
  }
}

void Peggy2ConwayEngine::InitializeGlider()
{
  Peggy2* currentGen = this->genMemory[this->currentGenIndex];
  currentGen->Clear();
  currentGen->WritePoint(0,0,1);
  currentGen->WritePoint(2,0,1);
  currentGen->WritePoint(2,1,1);
  currentGen->WritePoint(1,1,1);
  currentGen->WritePoint(1,2,1);
}

void Peggy2ConwayEngine::InitializeRPentomino()
{
  Peggy2* currentGen = this->genMemory[this->currentGenIndex];
  currentGen->Clear();
  currentGen->WritePoint(12,11,1);
  currentGen->WritePoint(13,11,1);
  currentGen->WritePoint(11,12,1);  
  currentGen->WritePoint(12,12,1);
  currentGen->WritePoint(12,13,1);
}

void Peggy2ConwayEngine::InitializeBlinker()
{
  Peggy2* currentGen = this->genMemory[this->currentGenIndex];
  currentGen->Clear();
  currentGen->WritePoint(12,11,1);
  currentGen->WritePoint(13,11,1);
  currentGen->WritePoint(14,11,1); 
}


unsigned short Peggy2ConwayEngine::isAlive(unsigned short currentState, unsigned short neighborCount)
{
  unsigned short newState = 0;

  // error cases - should do something else than clipping...
  if (neighborCount < 0) neighborCount = 0;
  if (neighborCount > 8) neighborCount = 8;
  
  // if (neighborCount < 2 || neighborCount > 3) newState = 0; // useless, already initialized to false
  if (neighborCount == 2) newState = currentState;
  if (neighborCount == 3) newState = 1;

  return newState;
}

unsigned short Peggy2ConwayEngine::getNeighborCount(unsigned short x, unsigned short y)
{
  unsigned short count = 0;
  Peggy2* currentGen = this->genMemory[this->currentGenIndex];

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

unsigned short Peggy2ConwayEngine::getCurrentCell(unsigned short x, unsigned short y)
{
  Peggy2* currentGen = this->genMemory[this->currentGenIndex];
  
  if (x == -1) x = 24;
  if (x == 25) x = 0;
  if (y == -1) y = 24;
  if (y == 25) y = 0;
  
  return currentGen->GetPoint(x,y);
}

bool Peggy2ConwayEngine::areIdentical(Peggy2 *gen1, Peggy2 *gen2)
{
  for(int i = 0; i < 25; i++)
	{
		if (gen1->buffer[i] != gen2->buffer[i])
		{
			return false;
		}
	}
	
	return true;
}