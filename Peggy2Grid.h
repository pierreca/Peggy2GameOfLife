#ifndef __PEGGY2GRID_H__
#define __PEGGY2GRID_H__

#include <Peggy2Serial.h>
#include "GridDimensions.h"

// On-device grid: a thin, fully-inlined wrapper around a Peggy2 framebuffer.
// Every method forwards directly to the underlying Peggy2 so there is no
// runtime cost compared to using Peggy2 directly (the README warns against a
// performance hit). The engine talks to this through the ConwayGrid typedef.
class Peggy2Grid
{
  public:
    void HardwareInit() { device.HardwareInit(); }
    void Clear() { device.Clear(); }
    void WritePoint(unsigned char x, unsigned char y, unsigned char value) { device.WritePoint(x, y, value); }
    unsigned char GetPoint(unsigned char x, unsigned char y) { return device.GetPoint(x, y); }
    void RefreshAll(unsigned char count) { device.RefreshAll(count); }

    bool Equals(const Peggy2Grid& other) const
    {
      for (int i = 0; i < ROWS; i++)
      {
        if (device.buffer[i] != other.device.buffer[i])
        {
          return false;
        }
      }
      return true;
    }

    // Escape hatch for code that needs the raw Peggy2 (e.g. display helpers
    // like Line() / PeggyWriter in the sketch). Only available on-device.
    Peggy2* Device() { return &device; }

  private:
    Peggy2 device;
};

#endif
