// Game of life for Peggy 2.0
// Pierre Cauchois
// 10/10/2015
#include <Peggy2Serial.h>
#include <PeggyWriter.h>
#include <SimpleTimer.h>
#include "StepCounter.h"
#include "Peggy2ConwayEngine.h"

#define STEP_PERIOD_MS 500

SimpleTimer timer;
StepCounter stepCounter;
Peggy2ConwayEngine* peggy2ConwayEngine;

PeggyWriter StepCountWriter;
bool displayStepCounter = false;

void GameOfLifeStep()
{
  if (displayStepCounter)
  {
    ShowCounterScreen(10);
    stepCounter.ResetCurrentCount();
    randomSeed(millis());
    peggy2ConwayEngine->Initialize(Random);
    stepCounter.IncrementCount();
    
    displayStepCounter = false;
  }
  else
  {
    peggy2ConwayEngine->ComputeNextGen();
    
    if (peggy2ConwayEngine->DetectLoop())
    {
      displayStepCounter = true;
    }
    else
    {
      peggy2ConwayEngine->CommitNextGen();
      stepCounter.IncrementCount();
    }
  }
}

void ShowCounterScreen(int timeInSeconds)
{
  Peggy2 *currentGen = peggy2ConwayEngine->GetCurrentFrame();
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
  peggy2ConwayEngine = new Peggy2ConwayEngine(4);
  peggy2ConwayEngine->Initialize(Blinker);
  stepCounter.IncrementCount();
  
  displayStepCounter = false;
  timer.setInterval(STEP_PERIOD_MS, GameOfLifeStep);
}

void loop()
{
  timer.run();
  peggy2ConwayEngine->GetCurrentFrame()->RefreshAll(1);
}
