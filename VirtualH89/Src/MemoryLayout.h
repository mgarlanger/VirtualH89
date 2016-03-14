///
/// \file MemoryLayout.h
///
/// A container for a 64K layout of memory, for addresses 0000-FFFF, in 8K pages.
///
/// \date Mar 05, 2016
/// \author Douglas Miller
///
#ifndef MEMORYLAYOUT_H_
#define MEMORYLAYOUT_H_

#include "Memory8K.h"
#include "Memory64K.h"
#include "NilMemory8K.h"

class MemoryLayout
{
  public:
    MemoryLayout() {
        int x;
        for (x = 0; x < 8; ++x)
        {
            memBnk[x] = new NilMemory8K(x << 13);
        }
    }
    void addPage(Memory8K* mem) {
        int a = (mem->getBase() >> 13) & 0x07;
        // error if not NULL?
        memBnk[a] = mem;
    }
    void addPageAt(Memory8K* mem, WORD adr) {
        int a = (adr >> 13) & 0x07;
        // error if not NULL?
        memBnk[a] = mem;
    }
    void addPage(Memory64K* mem64) {
        int x;
        for (x = 0; x < 8; ++x)
        {
            addPage(mem64->getPage(x << 13));
        }
    }
    inline Memory8K* getPage(WORD adr) {
        int a = (adr >> 13) & 0x07;
        // error if NULL?
        return memBnk[a];
    }
  protected:
    Memory8K* memBnk[8]; // 8 8K regions in 64K addr space
};

#endif // MEMORYLAYOUT_H_
