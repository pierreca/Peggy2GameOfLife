// Game of life for Peggy 2.0
// Pierre Cauchois
// 10/10/2015
#include <stdlib.h>
#include <Peggy2Serial.h>
#include <PeggyWriter.h>
#include <SimpleTimer.h>
#include "StepCounter.h"
#include "ConwayEngine.h"

#define STEP_PERIOD_MS 500

SimpleTimer timer;
StepCounter stepCounter;
ConwayEngine* conwayEngine;

PeggyWriter StepCountWriter;
bool displayStepCounter = false;

// Seed the same generator that InitializeRandom() draws from (stdlib rand()),
// from a genuinely varying source so each power-on starts differently. An
// unconnected analog pin (A0) reads electrical noise, which beats millis() (~0
// at startup). Must run before the first random frame is generated.
void SeedRandom()
{
  srand(analogRead(A0));
}

void GameOfLifeStep()
{
  if (displayStepCounter)
  {
    ShowCounterScreen(10);
    stepCounter.ResetCurrentCount();
    SeedRandom();
    conwayEngine->Initialize(Random);
    stepCounter.IncrementCount();
    
    displayStepCounter = false;
  }
  else
  {
    conwayEngine->ComputeNextGen();
    
    if (conwayEngine->DetectLoop())
    {
      displayStepCounter = true;
    }
    else
    {
      conwayEngine->CommitNextGen();
      stepCounter.IncrementCount();
    }
  }
}

void ShowCounterScreen(int timeInSeconds)
{
  Peggy2 *currentGen = conwayEngine->GetCurrentFrame()->Device();
  currentGen->Clear();
  char stepCountText[5];
  char maxStepCountText[5];
  
  StepCountWriter.drawCharacterSequence("LAST:", currentGen, 1, 0, false);
  StepCountWriter.drawCharacterSequence(stepCounter.GetCurrentCountString(), currentGen, 1, 6, false);
  currentGen->Line(0,12,24,12);
  StepCountWriter.drawCharacterSequence("MAX:", currentGen, 1, 14, false);
  StepCountWriter.drawCharacterSequence(stepCounter.GetMaxCountString(), currentGen, 1, 20, false);
  
  for (int i = 0; i < timeInSeconds * 100; i++) 
  {
    currentGen->RefreshAll(5);
    delay(10);
  }
}

void setup()
{
  conwayEngine = new ConwayEngine(4);
  SeedRandom();
  conwayEngine->Initialize(Random);
  stepCounter.IncrementCount();
  
  displayStepCounter = false;
  timer.setInterval(STEP_PERIOD_MS, GameOfLifeStep);
}

void loop()
{
  timer.run();
  conwayEngine->GetCurrentFrame()->RefreshAll(1);
}
