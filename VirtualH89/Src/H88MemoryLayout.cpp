///
///  \file H88MemoryLayout.cpp
///
///  \author Mark Garlanger
///  \date   May 8, 2016
///

#include "H88MemoryLayout.h"

#include "RAMemory8K.h"


H88MemoryLayout::H88MemoryLayout(shared_ptr<Memory8K> hdos): MemoryLayout()
{
    // ROM at 0x0000
    addPage(hdos);
    // in non-Org 0 mode the 48k of RAM is at 8K -> 56K
    for (int x = 1; x < 7; ++x)
    {
        addPage(make_shared<RAMemory8K>(pageToAddress(x)));
    }
}

H88MemoryLayout::H88MemoryLayout(shared_ptr<MemoryLayout> nonOrg0Layout): MemoryLayout()
{
    // Org-0 has the memory from 48K to 56K mapped to address 0.
    for (int x = 1; x < 6; ++x)
    {
        addPage(nonOrg0Layout->getPage(x));
    }

    //
    addPageAt(nonOrg0Layout->getPage(6), 0x0000);
}
