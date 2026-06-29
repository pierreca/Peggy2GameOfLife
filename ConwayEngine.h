#ifndef CONWAYENGINE_H
#define CONWAYENGINE_H

#include <stdint.h>
#include "ConwayGrid.h"

typedef enum InitFrame
{
	Random,
	Blinker,
	RPentomino,
	Glider
} InitFrame;

class ConwayEngine
{
	public:
		// loopHistorySize is how many recent generations are remembered for loop
		// detection. History is stored as compact per-generation hashes (not full
		// frames), so a deep window is cheap: it catches longer-period oscillators
		// than the old 4-frame ring could. The trade-off is a ~2^-32 chance that a
		// hash collision reports a loop one generation early, which for an
		// autonomous display only means an occasional slightly-early reseed.
		// A size of 0 disables loop detection (DetectLoop always returns false).
		ConwayEngine(unsigned short loopHistorySize);
		~ConwayEngine();

		// The engine owns raw heap buffers (frames + hash history) and frees them
		// in the destructor, so it must not be copied: a shallow copy would alias
		// those pointers and double-free. It is only ever held by pointer/reference.
		ConwayEngine(const ConwayEngine&) = delete;
		ConwayEngine& operator=(const ConwayEngine&) = delete;

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
		// Two frames are enough: one displayed (current) while the next is computed
		// (double buffering). Loop-detection history lives in loopHashes, not here.
		static const unsigned short frameCount = 2;
		unsigned short currentGenIndex;
		unsigned short nextGenIndex;
		ConwayGrid** genMemory;

		// Circular buffer of the most recent generation hashes (capacity
		// loopHistorySize). loopHashCount valid entries; loopHashHead is the next
		// write slot.
		uint32_t* loopHashes;
		unsigned short loopHistorySize;
		unsigned short loopHashCount;
		unsigned short loopHashHead;

		uint32_t hashFrame(ConwayGrid* frame);
		void recordHash(uint32_t hash);

		unsigned short isAlive(unsigned short currentState, unsigned short neighborCount);
		unsigned short getNeighborCount(int x, int y);
		unsigned short getCurrentCell(int x, int y);
};

#endif
