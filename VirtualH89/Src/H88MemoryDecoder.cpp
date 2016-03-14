///
/// \file H88MemoryDecoder.cpp
///
/// Basic memory layout of bare-bones H88 48K RAM system.
///
/// \date Mar 05, 2016
/// \author Douglas Miller
///
#include "H88MemoryDecoder.h"
#include "MemoryDecoder.h"

/// H88 48K Memory
H88MemoryDecoder::H88MemoryDecoder(MemoryLayout* h89_0):
    MemoryDecoder(1, 0) {
    addLayout(0, h89_0);
}
H88MemoryDecoder::~H88MemoryDecoder(){
}

void
H88MemoryDecoder::gppNewValue(BYTE gpo) {
    // This never gets called.
    curBank = 0;
}

void
H88MemoryDecoder::reset() {
    curBank = 0;
}

void
H88MemoryDecoder::addLayout(int ix, MemoryLayout* lo) {
    banks[0] = lo;
}
