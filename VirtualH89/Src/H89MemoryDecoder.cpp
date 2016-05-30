///
/// \file H89MemoryDecoder.cpp
///
/// H89 56K+ROM or 64K layout
///
/// \date Mar 05, 2016
/// \author Douglas Miller
///
#include "H89MemoryDecoder.h"

#include "RAMemory8K.h"
#include "SystemMemory8K.h"

using namespace std;

H89MemoryDecoder::H89MemoryDecoder(shared_ptr<SystemMemory8K> systemRam,
                                   MemoryLayout::MemorySize_t memSize): MemoryDecoder(2,
                                                                                      h89_gppOrg0Bit_c)
{
    // Two different Memory Layout for the H89 ORG-0 configuration
    MemoryLayout_ptr h89_0 = make_shared<MemoryLayout>();
    MemoryLayout_ptr h89_1 = make_shared<MemoryLayout>();

    // Add the ROMs/Floppydisk RAM
    h89_0->addPage(systemRam);

    // determine the number of banks based on memSize
    int numBanks = 0;
    switch (memSize)
    {
        case MemoryLayout::Mem_16k:
            numBanks = 2;
            break;

        case MemoryLayout::Mem_32k:
            numBanks = 4;
            break;

        case MemoryLayout::Mem_48k:
            numBanks = 6;
            break;

        case MemoryLayout::Mem_64k:
            numBanks = 8;
            break;

        case MemoryLayout::Mem_None:
        default:
            // \todo abort.
            break;
    }

    // Add all the fixed memory to both memory layouts
    for (int x = 1; x < numBanks; x++)
    {
        shared_ptr<Memory8K> mem = make_shared<RAMemory8K>(x << 13);

        h89_0->addPage(mem);
        h89_1->addPage(mem);
    }

    // handle the bank that handles bank 0 in the ORG-0 mode
    if (memSize == MemoryLayout::Mem_64k)
    {
        // With a full 64k, the 8K bank both backfills the ROM area (writes to
        // ROM cause write to the RAM) and is the first bank in the second layout.

        // lowest memory
        shared_ptr<Memory8K> rd6_0 = make_shared<RAMemory8K>(0x0000);

        // make sure it backfills the ROM area
        systemRam->overlayRAM(rd6_0);

        // add it to the second bank.
        h89_1->addPage(rd6_0);
    }
    else
    {
        // the last bank is either at the top of memory or at 0 depending on layout

        // now add the last memory bank that also is at address 0 when ORG 0 is enabled.
        shared_ptr<Memory8K> topMem = make_shared<RAMemory8K>(numBanks << 13);
        h89_0->addPage(topMem);

        // add to second layout at address 0.
        h89_1->addPageAt(topMem, 0);
    }

    // Store the memory layouts
    addLayout(0, h89_0);
    addLayout(1, h89_1);

    updateCurLayout(0);
}

H89MemoryDecoder::~H89MemoryDecoder()
{
}

void
H89MemoryDecoder::gppNewValue(BYTE gpo)
{
    updateCurLayout((gpo & h89_gppOrg0Bit_c) ? 1 : 0);
}
