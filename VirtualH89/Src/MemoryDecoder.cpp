///
///  \file MemoryDecoder.cpp
///
///  \author Mark Garlanger
///  \date   May 8, 2016
///

#include "MemoryDecoder.h"



MemoryDecoder::MemoryDecoder(int numBanks, BYTE gppBits): GppListener(gppBits),
                                                          curBank_m(0),
                                                          bankMask_m(numBanks - 1),
                                                          numBanks_m(numBanks)
{
    banks_m = new MemoryLayout*[numBanks];
    GppListener::addListener(this);
}

MemoryDecoder::~MemoryDecoder()
{

}
