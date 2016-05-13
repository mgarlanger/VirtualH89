///
/// \file HDOSMemory8K.h
///
/// 8K page of ROM and RAM for HDOS (H88). Tracks R/O and uninstalled segments of 1K.
/// Implements "write under ROM" if an 8K RAM page is added.
///
/// \date Mar 05, 2016
/// \author Douglas Miller
///
#ifndef HDOSMEMORY8K_H_
#define HDOSMEMORY8K_H_

#include "RAMemory8K.h"


class ROM;

class HDOSMemory8K: public RAMemory8K
{
  public:
    HDOSMemory8K();
    void overlayRAM(Memory8K* ram);
    void enableRAM(WORD base, WORD len);
    void writeProtect(WORD adr, WORD len);
    void writeEnable(WORD adr, WORD len);
    void installROM(ROM* rom);

    void writeByte(WORD adr, BYTE val) {
        // "write under ROM" accesses both DRAM and H17-RAM
        if (RAM != nullptr)
        {
            RAM->writeByte(adr, val);
        }
        if (maskRO)
        {
            int a = (adr >> 10) & 0x07;
            if ((maskRO & (1 << a)) != 0 || (maskInstalled & (1 << a)) == 0)
            {
                return;
            }
        }
        RAMemory8K::writeByte(adr, val);
    }

  private:
    unsigned int maskRO;
    unsigned int maskInstalled;
    Memory8K*    RAM;
};

#endif // HDOSMEMORY8K_H_
