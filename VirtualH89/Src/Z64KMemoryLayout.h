///
/// \file Z64KMemoryLayout.h
///
/// 16K add-on RAM to a basic 48K H88 layout to create a 64K RAM layout.
///
/// \date Mar 05, 2016
/// \author Douglas Miller
///
#ifndef Z64KMEMORYLAYOUT_H_
#define Z64KMEMORYLAYOUT_H_

#include "MemoryLayout.h"
#include "RAMemory8K.h"

class Z64KMemoryLayout: public MemoryLayout
{
  public:
    Z64KMemoryLayout(MemoryLayout* rom): MemoryLayout() {
        int       x;
        Memory8K* rd6_0 = new RAMemory8K(0x0000);
        Memory8K* rd6_1 = new RAMemory8K(0xe000);
        rom->addPage(rd6_1);
        addPage(rd6_0);
        for (x = 1; x < 7; ++x)
        {
            addPage(rom->getPage(x << 13));
        }
        addPage(rd6_1);
    }
};

#endif // Z64KMEMORYLAYOUT_H_
