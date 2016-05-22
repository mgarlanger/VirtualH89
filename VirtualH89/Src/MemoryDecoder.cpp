///
///  \file MemoryDecoder.cpp
///
///  \author Mark Garlanger
///  \date   May 8, 2016
///

#include "MemoryDecoder.h"



MemoryDecoder::MemoryDecoder(int  numBanks,
                             BYTE gppBits): GppListener(gppBits),
                                            curBank_m(0),
                                            bankMask_m(numBanks - 1),
                                            numBanks_m(numBanks),
                                            curLayout_m(nullptr)
{
    GppListener::addListener(this);
    banks_m.resize(numBanks);
    for (shared_ptr<MemoryLayout> bank : banks_m)
    {
        bank = nullptr;
    }
    curLayout_m = banks_m[curBank_m];
}

MemoryDecoder::~MemoryDecoder()
{

}

void
MemoryDecoder::reset()
{
    updateCurBank(0);
}

void
MemoryDecoder::addLayout(int                      ix,
                         shared_ptr<MemoryLayout> lo)
{
    if (ix >= 0 && ix < numBanks_m)
    {
        banks_m[ix] = lo;
    }
    curLayout_m = banks_m[curBank_m];

}

void
MemoryDecoder::updateCurBank(BYTE bank)
{
    curBank_m   = bank;
    curLayout_m = banks_m[curBank_m];
}
