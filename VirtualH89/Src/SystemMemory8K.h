///
/// \file SystemMemory8K.h
///
/// 8K page of ROM and RAM for HDOS (H88). Tracks R/O and uninstalled segments of 1K.
/// Implements "write under ROM" if an 8K RAM page is added.
///
/// \date Mar 05, 2016
/// \author Douglas Miller
///
#ifndef SYSTEMMEMORY8K_H_
#define SYSTEMMEMORY8K_H_

#include "RAMemory8K.h"


class ROM;

class SystemMemory8K: public RAMemory8K
{
  public:
    SystemMemory8K();
    void overlayRAM(std::shared_ptr<Memory8K> ram);
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
    unsigned int                 maskRO;
    unsigned int                 maskInstalled;
    std::shared_ptr<Memory8K>    RAM;
};

typedef std::shared_ptr<SystemMemory8K> systemMem_ptr;

#endif // SYSTEMMEMORY8K_H_
