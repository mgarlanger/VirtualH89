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
H88MemoryDecoder::H88MemoryDecoder(shared_ptr<MemoryLayout> h89_0): MemoryDecoder(2,
                                                                                  h89_gppOrg0Bit_c)
{
    addLayout(0, h89_0);
    shared_ptr<MemoryLayout> h89_1 = make_shared<H88MemoryLayout>(h89_0);

    addLayout(1, h89_1);
}

H88MemoryDecoder::~H88MemoryDecoder()
{
}

void
H88MemoryDecoder::gppNewValue(BYTE gpo)
{
    updateCurBank((gpo & h89_gppOrg0Bit_c) ? 1 : 0);

}
