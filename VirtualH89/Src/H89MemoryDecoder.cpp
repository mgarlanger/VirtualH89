///
/// \file H89MemoryDecoder.cpp
///
/// H89 56K+ROM or 64K layout
///
/// \date Mar 05, 2016
/// \author Douglas Miller
///
#include "H89MemoryDecoder.h"
#include "MemoryDecoder.h"
#include "Z64KMemoryLayout.h"

/// H89 64K Memory
H89MemoryDecoder::H89MemoryDecoder(MemoryLayout* h89_0):
    MemoryDecoder(2, h89_gppOrg0Bit_c) {
    MemoryLayout* h89_1 = new Z64KMemoryLayout(h89_0);
    addLayout(0, h89_0);
    addLayout(1, h89_1);
}
H89MemoryDecoder::~H89MemoryDecoder(){
}

void
H89MemoryDecoder::gppNewValue(BYTE gpo) {
    curBank = ((gpo & h89_gppOrg0Bit_c) ? 1 : 0);
}

void
H89MemoryDecoder::reset() {
    curBank = 0;
}

void
H89MemoryDecoder::addLayout(int ix, MemoryLayout* lo) {
    if (ix >= 0 && ix < numBnks)
    {
        banks[ix] = lo;
    }
}
