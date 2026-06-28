#ifndef __PEGGY2CONWAYENGINE_H__
#define __PEGGY2CONWAYENGINE_H__

#include "ConwayGrid.h"

typedef enum InitFrame
{
	Random,
	Blinker,
	RPentomino,
	Glider
} InitFrame;

class Peggy2ConwayEngine
{
	public:
		Peggy2ConwayEngine(unsigned short genMemorySize);
		void Initialize(InitFrame initializationType);
		void ComputeNextGen();
		void CommitNextGen();
		ConwayGrid* GetCurrentFrame();
		bool DetectLoop();

		void InitializeRandom();
		void InitializeBlinker();
		void InitializeRPentomino();
		void InitializeGlider();
	private:
		unsigned short currentGenIndex;
		unsigned short nextGenIndex;
		ConwayGrid** genMemory;
		unsigned short genMemorySize;

		unsigned short isAlive(unsigned short currentState, unsigned short neighborCount);
		unsigned short getNeighborCount(int x, int y);
		unsigned short getCurrentCell(int x, int y);
};

#endif
