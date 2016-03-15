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
#include "Memory8K.h"

class HDOSMemory8K: public RAMemory8K
{
  public:
    HDOSMemory8K():
        RAMemory8K(0x0000),
        maskRO(0),
        maskInstalled(0),
        RAM(NULL)
    {
    }
    void overlayRAM(Memory8K* ram) {
        RAM = ram;
    }
    void enableRAM(WORD base, WORD len) {
        WORD adr = base & 0x1fff;

        if (adr + len > sizeof(mem))
        {
            // error? or just trim?
            len = sizeof(mem) - adr;
        }
        int a = (adr >> 10) & 0x07;
        int n = a + (((len + 0x03ff) >> 10) & 0x07);
        // TODO: find more-elegant way
        for (; a < n; ++a)
        {
            maskInstalled |= (1 << a);
        }
    }
    void writeProtect(WORD adr, WORD len) {
        int a = (adr >> 10) & 0x07;
        int n = a + (((len + 0x03ff) >> 10) & 0x07);

        // TODO: find more-elegant way
        for (; a < n && a < 8; ++a)
        {
            maskRO |= (1 << a);
        }
    }
    void writeEnable(WORD adr, WORD len) {
        int a = (adr >> 10) & 0x07;
        int n = a + (((len + 0x03ff) >> 10) & 0x07);

        // TODO: find more-elegant way
        for (; a < n && a < 8; ++a)
        {
            maskRO &= ~(1 << a);
        }
    }
    void installROM(ROM* rom) {
        WORD adr = rom->getBase() & 0x1fff;
        WORD len = rom->getSize();

        if (adr + len > sizeof(mem))
        {
            // error? or just trim?
            len = sizeof(mem) - adr;
        }
        memcpy(&mem[adr], rom->getImage(), len);
        int a = (adr >> 10) & 0x07;
        int n = a + (((len + 0x03ff) >> 10) & 0x07);
        // TODO: find more-elegant way
        for (; a < n; ++a)
        {
            maskRO        |= (1 << a);
            maskInstalled |= (1 << a);
        }
    }
    void writeByte(WORD adr, BYTE val) {
        // "write under ROM" accesses both DRAM and H17-RAM
        if (RAM != NULL)
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
