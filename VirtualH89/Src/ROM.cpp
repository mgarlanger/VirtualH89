/// \file ROM.cpp
///
/// \brief Implementation of virtual ROM.
///
/// \date Mar 7, 2009
/// \author Mark Garlanger
///


#include "ROM.h"

#include <cassert>
#include <fstream>
#include "logger.h"

ROM::ROM(int size): Memory(size),
                    data_m(0)
{
    debugss(ssROM, INFO, "%s: Creating ROM: %d\n", __FUNCTION__, size_m);

    data_m = new BYTE[size_m];
}

ROM::~ROM()
{
    debugss(ssROM, INFO, "%s: Destroying ROM\n", __FUNCTION__);

    delete[] data_m;
    data_m = 0;
    size_m = 0;
}

ROM*
ROM::getROM(const char* filename, WORD addr)
{
    std::ifstream     file;
    unsigned int      fileSize;
    BYTE*             buf;
    ROM*              rom = NULL;

    file.open(filename, std::ios::binary);

    if (!file.is_open())
    {
        debugss(ssROM, ERROR, "%s: ROM image \"%s\" cannot be opened\n", __FUNCTION__, filename);
        return NULL;
    }

    file.seekg(0, std::ios::end);
    fileSize = (unsigned int) file.tellg();
    file.seekg(0, std::ios::beg);

    if (fileSize != 2048 && fileSize != 4096)
    {
        debugss(ssROM, ERROR, "%s: ROM image \"%s\" has invalid size %u\n", __FUNCTION__, filename,
                fileSize);
        return NULL;
    }

    buf = new BYTE[fileSize];
    file.read((char*) buf, fileSize);
    file.close();

    rom = new ROM(fileSize);
    rom->setBaseAddress(addr);
    rom->initialize(buf, fileSize);
    delete[] buf;
    return rom;
}

void
ROM::initialize(BYTE* block, WORD size)
{
    debugss(ssROM, INFO, "%s: size(%d)\n", __FUNCTION__, size);

    if (size > size_m)
    {
        // error attempting to store too much
        debugss(ssROM, ERROR, "%s: size to big size(%d) size_m(%d)\n",
                __FUNCTION__, size, size_m);
        size = size_m;
    }

    for (int i = 0; i < size; i++)
    {
        debugss(ssROM, VERBOSE, "%s: block[%d] = %d\n", __FUNCTION__, i, block[i]);
        data_m[i] = block[i];
    }
}


void
ROM::writeByte(WORD addr, BYTE val)
{
    // can't write to ROM.
    /// \todo update to set the RAM.
    debugss(ssROM, INFO, "%s: Attempting to write to ROM [%d] = %d\n",
            __FUNCTION__, addr, val);
    return;
}

BYTE
ROM::readByte(WORD addr)
{
    BYTE val    = 0;
    WORD offset = addr - baseAddress_m;

    if ((addr < baseAddress_m) || (offset >= size_m))
    {
        debugss(ssROM, ERROR, "%s: Invalid address: %d (base: %d/size: %d)\n", __FUNCTION__,
                addr, baseAddress_m, size_m);
        assert((addr >= baseAddress_m) && (offset < size_m));

    }

    else
    {
        val = data_m[offset];
    }

    debugss(ssROM, ALL, "%s: addr[%d] = %d\n", __FUNCTION__, addr, val);
    return (val);
}
