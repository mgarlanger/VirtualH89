///
/// \file H89MemoryDecoder.cpp
///
/// H89 56K+ROM or 64K layout
///
/// \date Mar 05, 2016
/// \author Douglas Miller
///
#include "H89MemoryDecoder.h"

#include "Z64KMemoryLayout.h"

/// H89 64K Memory
H89MemoryDecoder::H89MemoryDecoder(shared_ptr<MemoryLayout> h89_0): MemoryDecoder(2,
                                                                                  h89_gppOrg0Bit_c)
{
    addLayout(0, h89_0);
    shared_ptr<MemoryLayout> h89_1 = make_shared<Z64KMemoryLayout>(h89_0);
    addLayout(1, h89_1);
}

H89MemoryDecoder::~H89MemoryDecoder()
{
}

void
H89MemoryDecoder::gppNewValue(BYTE gpo)
{
    updateCurBank((gpo & h89_gppOrg0Bit_c) ? 1 : 0);
}
