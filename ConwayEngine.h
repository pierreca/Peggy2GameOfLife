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

// Tunables for the three independent termination heuristics. A plain aggregate
// (no constructor, no default member initializers) so it stays brace-initable
// under the AVR toolchain's gnu++11 as well as the host builds; every call site
// sets all three fields explicitly. Any single field set to 0 disables its
// heuristic, so they can be mixed and matched freely.
struct EngineConfig
{
	// Recent generations remembered for loop detection, stored as compact
	// per-generation hashes (not full frames), so a deep window is cheap and
	// catches longer-period oscillators than a small frame ring could. The
	// trade-off is a ~2^-32 chance a hash collision reports a loop one generation
	// early (a harmless slightly-early reseed). 0 disables loop detection.
	unsigned short loopHistorySize;

	// Max-generation watchdog (issue #8): force a reseed once a run reaches this
	// many committed generations even if nothing else fired. The backstop for
	// gliders/spaceships and very-long-period or chaotic fields that never recur
	// within loopHistorySize. 0 disables the watchdog.
	unsigned long maxGenerations;

	// Population-stagnation window (issue #9): treat this many consecutive
	// generations of unchanged live-cell count as a settled/stuck field and force
	// a reseed. Complementary to loop detection (which needs an exact state match)
	// and the watchdog (a fixed generation cap). 0 disables stagnation detection.
	unsigned short populationWindow;
};

class ConwayEngine
{
	public:
		// Full configuration: see EngineConfig for what each tunable controls and
		// how a 0 disables it.
		explicit ConwayEngine(const EngineConfig& config);

		// Convenience constructor: configure loop detection only, with the
		// max-generation watchdog and population-stagnation detector disabled.
		// loopHistorySize behaves exactly as EngineConfig::loopHistorySize.
		explicit ConwayEngine(unsigned short loopHistorySize);
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

		// The three termination heuristics, each independent and each a no-op when
		// its EngineConfig tunable is 0. The orchestrator reseeds when any fire.
		bool DetectLoop();          // current state recurs within loopHistorySize
		bool WatchdogExpired();     // run reached maxGenerations committed gens
		bool PopulationStagnant();  // population unchanged for populationWindow gens

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

		// Max-generation watchdog state: generationCount is the number of committed
		// generations in the current run (1 right after Initialize, then +1 per
		// CommitNextGen). The watchdog fires once it reaches maxGenerations.
		unsigned long maxGenerations;
		unsigned long generationCount;

		// Population-stagnation state, tracked in O(1): lastPopulation is the live
		// cell count of the most recently recorded generation (-1 before any), and
		// stagnantRun is how many consecutive generations have shared it. Detection
		// fires once stagnantRun reaches populationWindow. A run-length counter
		// rather than a window buffer keeps this free of per-window RAM on the AVR.
		unsigned short populationWindow;
		int lastPopulation;
		unsigned short stagnantRun;

		uint32_t hashFrame(ConwayGrid* frame);
		void recordHash(uint32_t hash);
		unsigned short countPopulation(ConwayGrid* frame);
		void recordPopulation(unsigned short population);

		unsigned short isAlive(unsigned short currentState, unsigned short neighborCount);
		unsigned short getNeighborCount(int x, int y);
		unsigned short getCurrentCell(int x, int y);
};

#endif
