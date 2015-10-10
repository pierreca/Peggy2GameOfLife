#ifndef __PEGGY2CONWAYENGINE_H__
#define __PEGGY2CONWAYENGINE_H__

#define ROWS 25
#define COLUMNS 25

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
		Peggy2* GetCurrentFrame();
		bool DetectLoop();
		
		void InitializeRandom();
		void InitializeBlinker();
		void InitializeRPentomino();
		void InitializeGlider();
	private:
		unsigned short currentGenIndex;
		unsigned short nextGenIndex;
		Peggy2** genMemory;
		unsigned short genMemorySize;
		
		unsigned short isAlive(unsigned short currentState, unsigned short neighborCount);
		unsigned short getNeighborCount(unsigned short x, unsigned short y);
		unsigned short getCurrentCell(unsigned short x, unsigned short y);
		bool areIdentical(Peggy2 *gen1, Peggy2 *gen2);
};

#endif
