///
/// \file H88MemoryLayout.h
///
/// H88 48K RAM+ROM layout
///
/// \date Mar 05, 2016
/// \author Douglas Miller
///
#ifndef H88MEMORYLAYOUT_H_
#define H88MEMORYLAYOUT_H_

#include "MemoryLayout.h"
#include "Memory8K.h"
#include "RAMemory8K.h"

class H88MemoryLayout: public MemoryLayout
{
  public:
    H88MemoryLayout(Memory8K* hdos): MemoryLayout() {
        addPage(hdos);
        int x;
        for (x = 1; x < 7; ++x)
        {
            addPage(new RAMemory8K(x << 13));
        }
    }
};

#endif // H88MEMORYLAYOUT_H_
