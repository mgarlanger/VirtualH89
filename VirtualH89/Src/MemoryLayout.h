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

/// \cond
#include <memory>
/// \endcond

class Memory8K;

class MemoryLayout
{
  public:
    enum MemorySize_t
    {
        Mem_None,
        Mem_16k,
        Mem_32k,
        Mem_48k,
        Mem_64k
    };

    MemoryLayout();

    void addPage(std::shared_ptr<Memory8K> mem);
    void addPageAt(std::shared_ptr<Memory8K> mem,
                   WORD                      adr);

    inline std::shared_ptr<Memory8K> getPageByAddress(WORD address)
    {
        // error if NULL?
        return memPage_m[addressToPage(address)];
    }

    inline std::shared_ptr<Memory8K> getPage(BYTE page)
    {
        // error if NULL?
        if (page >= numPages_c)
        {
            // TODO - add something for out-of-range
        }

        return memPage_m[page & 0x7];
    }
    static const BYTE pageShiftFactor_c = 13;
    static const BYTE numPages_c        = 8;

    static BYTE addressToPage(WORD address)
    {
        return (address >> pageShiftFactor_c) & 0x07;
    }

    static WORD pageToAddress(BYTE page)
    {
        return page << pageShiftFactor_c;
    }

  protected:

    std::shared_ptr<Memory8K> memPage_m[numPages_c]; // 8 8K regions in 64K addr space
};

typedef std::shared_ptr<MemoryLayout> MemoryLayout_ptr;

#endif // MEMORYLAYOUT_H_
