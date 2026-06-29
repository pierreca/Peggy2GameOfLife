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

// How many recent generations to remember for loop detection. Stored as compact
// 4-byte hashes (not full frames), so this depth is cheap on the AVR's limited
// RAM (LOOP_HISTORY * 4 bytes) yet catches far longer oscillator periods than
// the original 4-frame ring. Tune up for more coverage, down to save RAM.
#define LOOP_HISTORY 32

// Max-generation watchdog (issue #8): force a reseed after this many generations
// even if no loop is detected, so a glider/spaceship drifting around the torus
// (or a chaotic field longer than LOOP_HISTORY) can't leave the board looking
// frozen forever. At STEP_PERIOD_MS this caps a run at ~MAX_GENERATIONS/2
// seconds. 0 would disable the watchdog.
#define MAX_GENERATIONS 1000

// Population-stagnation window (issue #9): reseed once the live-cell count has
// stayed unchanged for this many consecutive generations, catching settled or
// stuck fields whose exact state never recurs within LOOP_HISTORY. Kept well
// above LOOP_HISTORY so ordinary oscillators are caught by loop detection first.
// 0 would disable stagnation detection.
#define POPULATION_WINDOW 150

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

    // Reseed when the field repeats (loop), drifts forever (watchdog), or has
    // settled into an unchanging population (stagnation) -- any one is enough.
    if (conwayEngine->DetectLoop() ||
        conwayEngine->WatchdogExpired() ||
        conwayEngine->PopulationStagnant())
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
  conwayEngine = new ConwayEngine(EngineConfig{LOOP_HISTORY, MAX_GENERATIONS, POPULATION_WINDOW});
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
