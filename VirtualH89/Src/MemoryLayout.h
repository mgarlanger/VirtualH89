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

#include "h89Types.h"

#include <memory>

class Memory8K;
class Memory64K;

using namespace std;

class MemoryLayout
{
  public:
    MemoryLayout();
    void addPage(shared_ptr<Memory8K> mem);
    void addPageAt(shared_ptr<Memory8K> mem, WORD adr);
    void addPage(Memory64K* mem64);

    inline shared_ptr<Memory8K> getPageByAddress(WORD address)
    {
        // error if NULL?
        return memPage_m[addressToPage(address)];
    }
    inline shared_ptr<Memory8K> getPage(BYTE page)
    {
        // error if NULL?
        if (page >= numPages_c)
        {
            // TODO - add something for out-of-range
        }

        return memPage_m[page & 0x7];
    }

  protected:
    static const BYTE pageShiftFactor_c = 13;
    static const BYTE numPages_c        = 8;

    BYTE addressToPage(WORD address)
    {
        return (address >> pageShiftFactor_c) & 0x07;
    }
    WORD pageToAddress(BYTE page)
    {
        return page << pageShiftFactor_c;
    }

    shared_ptr<Memory8K> memPage_m[numPages_c]; // 8 8K regions in 64K addr space
};

#endif // MEMORYLAYOUT_H_
