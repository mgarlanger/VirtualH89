///
/// \file Memory64K.h
///
/// A convenience container for 8 8K pages of memory, addressed 0000-E000.
///
/// \date Mar 05, 2016
/// \author Douglas Miller
///
#ifndef MEMORY64K_H_
#define MEMORY64K_H_

#include "Memory8K.h"
#include "RAMemory8K.h"

class Memory64K
{
  public:
    Memory64K() {
        int x;

        // for 64K all base addresses are defined
        for (x = 0; x < 8; ++x)
        {
            mem64k[x] = new RAMemory8K(x << 13);
        }
    }
    Memory8K* getPage(WORD adr) {
        int a = ((adr >> 13) & 0x07);
        return mem64k[a];
    }
  private:
    Memory8K* mem64k[8];
};

#endif // MEMORY64K_H_
