///
///  \file MemoryLayout.cpp
///
///  \author Mark Garlanger
///  \date   May 8, 2016
///

#include "MemoryLayout.h"

#include "NilMemory8K.h"

using namespace std;


MemoryLayout::MemoryLayout()
{
    for (int x = 0; x < 8; ++x)
    {
        memPage_m[x] = make_shared<NilMemory8K>(pageToAddress(x));
    }
}

void
MemoryLayout::addPage(shared_ptr<Memory8K> mem)
{
    // error if not NULL?
    memPage_m[addressToPage(mem->getBase())] = mem;
}

void
MemoryLayout::addPageAt(shared_ptr<Memory8K> mem, WORD address)
{
    // error if not NULL?
    memPage_m[addressToPage(address)] = mem;
}
