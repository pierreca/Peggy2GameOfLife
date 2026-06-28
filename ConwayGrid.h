#ifndef __CONWAYGRID_H__
#define __CONWAYGRID_H__

#include "GridDimensions.h"

// Compile-time grid selection. The Conway engine only ever refers to
// "ConwayGrid"; which concrete implementation it resolves to is decided here.
// This keeps the abstraction zero-cost (a typedef, no virtual dispatch).
//
//   - Host builds define CONWAY_HOST_BUILD and get the in-memory HostGrid.
//   - On-device builds get the Peggy2-backed Peggy2Grid.
#if defined(CONWAY_HOST_BUILD)
#include "HostGrid.h"
typedef HostGrid ConwayGrid;
#else
#include "Peggy2Grid.h"
typedef Peggy2Grid ConwayGrid;
#endif

#endif
