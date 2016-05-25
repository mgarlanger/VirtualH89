///
///  \file MemoryDecoder.cpp
///
///  \author Mark Garlanger
///  \date   May 8, 2016
///

#include "MemoryDecoder.h"


#include "H88MemoryDecoder.h"
#include "H89MemoryDecoder.h"
#include "MMS77318MemoryDecoder.h"
#include <string>
#include <memory>

using namespace std;


MemoryDecoder::MemoryDecoder(int  numLayouts,
                             BYTE gppBits): GppListener(gppBits),
                                            curLayoutNum_m(0),
                                            layoutMask_m(numLayouts - 1),
                                            numLayouts_m(numLayouts),
                                            curLayout_m(nullptr)
{
    GppListener::addListener(this);
    layouts_m.resize(numLayouts);
    for (MemoryLayout_ptr layout : layouts_m)
    {
        layout = nullptr;
    }
    curLayout_m = layouts_m[curLayoutNum_m];
}

MemoryDecoder::~MemoryDecoder()
{

}

void
MemoryDecoder::reset()
{
    updateCurLayout(0);
}

void
MemoryDecoder::addLayout(int              ix,
                         MemoryLayout_ptr lo)
{
    if (ix >= 0 && ix < numLayouts_m)
    {
        layouts_m[ix] = lo;
    }
    curLayout_m = layouts_m[curLayoutNum_m];

}

void
MemoryDecoder::updateCurLayout(BYTE layout)
{
    curLayoutNum_m = layout;
    curLayout_m    = layouts_m[curLayoutNum_m];
}


shared_ptr<MemoryDecoder>
MemoryDecoder::createMemoryDecoder(string                     type,
                                   shared_ptr<SystemMemory8K> sysMem,
                                   MemoryLayout::MemorySize_t memSize)
{

    shared_ptr<MemoryDecoder> memDecoder = nullptr;
    if (type == "MMS77318")
    {
        memDecoder = make_shared<MMS77318MemoryDecoder>(sysMem);
    }
    else if (type == "H89")
    {
        memDecoder = make_shared<H89MemoryDecoder>(sysMem, memSize);
    }
    else
    {
        memDecoder = make_shared<H88MemoryDecoder>(sysMem, memSize);
    }

    return memDecoder;
}

int
MemoryDecoder::numLayouts()
{
    return numLayouts_m;
}
