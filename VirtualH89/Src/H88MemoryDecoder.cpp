///
/// \file H88MemoryDecoder.cpp
///
/// Basic memory layout of bare-bones H88 48K RAM system.
///
/// \date Mar 05, 2016
/// \author Douglas Miller
///
#include "H88MemoryDecoder.h"


#include "H88MemoryLayout.h"

/// H88 48K Memory
H88MemoryDecoder::H88MemoryDecoder(MemoryLayout* h89_0): MemoryDecoder(2,
                                                                       h89_gppOrg0Bit_c)
{
    addLayout(0, h89_0);
    MemoryLayout* h89_1 = new H88MemoryLayout(h89_0);

    addLayout(1, h89_1);
}

H88MemoryDecoder::~H88MemoryDecoder()
{
}

void
H88MemoryDecoder::gppNewValue(BYTE gpo)
{
    curBank_m = ((gpo & h89_gppOrg0Bit_c) ? 1 : 0);
}

void
H88MemoryDecoder::reset()
{
    curBank_m = 0;
}

void
H88MemoryDecoder::addLayout(int ix, MemoryLayout* lo)
{
    if (ix >= 0 && ix < numBanks_m)
    {
        banks_m[ix] = lo;
    }
}
