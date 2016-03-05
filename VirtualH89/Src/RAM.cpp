/// \file RAM.cpp
///
/// \date Mar 7, 2009
/// \author Mark Garlanger
///

#include "RAM.h"

#include "logger.h"

/// \todo - Should I just make size the number of KB, instead of bytes?
RAM::RAM(int size): Memory(size),
                    data_m(0),
                    writeProtect_m(false)
{
    debugss(ssRAM, INFO, "%s: Creating RAM: %d\n", __FUNCTION__, size_m);

    data_m = new BYTE[size_m];
}

RAM::~RAM()
{
    debugss(ssRAM, INFO, "%s: Destroying RAM\n", __FUNCTION__);

    delete[] data_m;
    data_m = 0;
    size_m = 0;
}


void
RAM::writeByte(WORD addr,
               BYTE val)
{
    debugss(ssRAM, ALL, "%s: addr(%d) - %d\n", __FUNCTION__, addr, val);

    if (writeProtect_m)
    {
        debugss(ssRAM, INFO, "%s: attempting to write to protected RAM - addr(%d) - %d\n",
                __FUNCTION__, addr, val);
        return;
    }

    WORD offset = addr - baseAddress_m;

    if ((addr < baseAddress_m) || (offset >= size_m))
    {
        debugss(ssRAM, ERROR, "%s: Invalid address: %d (base: %d/size: %d)\n",
                __FUNCTION__, addr, baseAddress_m, size_m);
    }
    else
    {
        data_m[offset] = val;
    }
}

BYTE
RAM::readByte(WORD addr)
{
    BYTE val    = 0;
    WORD offset = addr - baseAddress_m;

    /// \todo - determine if this would be used for wrap-around at 64k..
    if ((addr < baseAddress_m) || (offset >= size_m))
    {
        debugss(ssRAM, ERROR, "%s: Invalid address: %d (base: %d/size: %d)\n",
                __FUNCTION__, addr, baseAddress_m, size_m);
    }
    else
    {
        val = data_m[offset];
    }

    debugss(ssRAM, ALL, "%s: addr(%d) - %d\n", __FUNCTION__, addr, val);

    return (val);
}


void
RAM::writeProtect(bool wp)
{
    writeProtect_m = wp;
}
