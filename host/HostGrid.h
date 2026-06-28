#ifndef __HOSTGRID_H__
#define __HOSTGRID_H__

#include "GridDimensions.h"

// Host grid: a plain in-memory bitmap that mirrors the cell/dimension/buffer
// operations the engine needs, with no Arduino or Peggy2 dependency. It lets
// the Conway engine compile and run on a development machine for testing.
//
// One unsigned long per row holds COLUMNS bits (COLUMNS <= 32). Out-of-range
// reads return 0 (dead) and out-of-range writes are ignored so the engine's
// neighbour scanning can probe past the edges without invoking undefined
// behaviour; on-device the Peggy2 grid keeps its original behaviour.
class HostGrid
{
  public:
    HostGrid() { Clear(); }

    void HardwareInit() { Clear(); }

    void Clear()
    {
      for (int i = 0; i < ROWS; i++)
      {
        buffer[i] = 0;
      }
    }

    void WritePoint(unsigned char x, unsigned char y, unsigned char value)
    {
      if (x >= COLUMNS || y >= ROWS)
      {
        return;
      }
      if (value)
      {
        buffer[y] |= (1UL << x);
      }
      else
      {
        buffer[y] &= ~(1UL << x);
      }
    }

    unsigned char GetPoint(unsigned char x, unsigned char y)
    {
      if (x >= COLUMNS || y >= ROWS)
      {
        return 0;
      }
      return (unsigned char)((buffer[y] >> x) & 1UL);
    }

    void RefreshAll(unsigned char /*count*/) {}

    bool Equals(const HostGrid& other) const
    {
      for (int i = 0; i < ROWS; i++)
      {
        if (buffer[i] != other.buffer[i])
        {
          return false;
        }
      }
      return true;
    }

  private:
    unsigned long buffer[ROWS];
};

#endif
