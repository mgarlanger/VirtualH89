///
///  \file Z64KMemoryLayout.cpp
///
///  \author Mark Garlanger
///  \date   May 8, 2016
///

#include "Z64KMemoryLayout.h"

#include "RAMemory8K.h"


Z64KMemoryLayout::Z64KMemoryLayout(shared_ptr<MemoryLayout> rom): MemoryLayout()
{

    shared_ptr<Memory8K> rd6_0 = make_shared<RAMemory8K>(0x0000);
    shared_ptr<Memory8K> rd6_1 = make_shared<RAMemory8K>(0xe000);
    rom->addPage(rd6_1);
    addPage(rd6_0);
    for (int x = 1; x < 7; ++x)
    {
        addPage(rom->getPage(x));
    }
    addPage(rd6_1);
}
