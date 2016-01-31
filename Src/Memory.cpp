/// \file Memory.cpp
///
/// \date Mar 7, 2009
/// \author Mark Garlanger
///

#include "Memory.h"
#include "logger.h"

Memory::Memory(int size): baseAddress_m(0),
    size_m(size)
{

}

Memory::~Memory()
{

}

WORD Memory::getBaseAddress(void)
{
    debugss(ssMEM, INFO, "%s: %d\n", __FUNCTION__, baseAddress_m);
    return baseAddress_m;
}

void Memory::setBaseAddress(WORD addr)
{
    debugss(ssMEM, INFO, "%s: %d\n", __FUNCTION__, addr);
    baseAddress_m = addr;
}

int Memory::getSize(void)
{
    debugss(ssMEM, INFO, "%s: %d\n", __FUNCTION__, size_m);
    return size_m;
}

