///
/// \file H88MemoryDecoder.cpp
///
/// Basic memory layout of bare-bones H88 48K RAM system.
///
/// \date Mar 05, 2016
/// \author Douglas Miller
///
#include "H88MemoryDecoder.h"


#include "Memory8K.h"
#include "SystemMemory8K.h"

#include "logger.h"

using namespace std;

/// H88 without support for ORG-0
H88MemoryDecoder::H88MemoryDecoder(std::shared_ptr<SystemMemory8K>  systemRam,
                                   MemoryLayout::MemorySize_t       memSize): MemoryDecoder(
                                                                                  NumMemoryLayouts_c,
                                                                                  gppMask_c)
{
    // only one layout if H89 doesn't have ORG-0
    MemoryLayout_ptr h89_0 = make_shared<MemoryLayout>();

    // addLayout(0, make_shared<H88MemoryLayout>(systemRam, memSize));
    // ROM at 0x0000
    h89_0->addPage(systemRam);

    unsigned numBanks;

    switch (memSize)
    {
        case MemoryLayout::Mem_16k:
            debugss(ssMEM, INFO, "16k memory specified\n");
            numBanks = 2;
            break;

        case MemoryLayout::Mem_32k:
            debugss(ssMEM, INFO, "32k memory specified\n");
            numBanks = 4;
            break;

        case MemoryLayout::Mem_48k:
            debugss(ssMEM, INFO, "48k memory specified\n");
            numBanks = 6;
            break;

        case MemoryLayout::Mem_64k:
            debugss(ssMEM, ERROR, "Invalid 64k memory specified\n");
            numBanks = 6;
            break;

        default:
            debugss(ssMEM, ERROR, "Invalid memory specified\n");
            numBanks = 6;
            break;
    }

    for (int x = 0; x < numBanks; ++x)
    {
        h89_0->addPage(make_shared<RAMemory8K>(MemoryLayout::pageToAddress(x + 1)));
    }

    addLayout(0, h89_0);
}

H88MemoryDecoder::~H88MemoryDecoder()
{
}

void
H88MemoryDecoder::gppNewValue(BYTE)
{
    // do nothing, this memory decoder doesn't support ORG-0 functionality

}
