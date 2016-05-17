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

ROM::ROM(int size):
    data_m(0),
    base_m(0),
    size_m(size)
{
    debugss(ssROM, INFO, "Creating ROM: %d\n", size_m);

    data_m = new BYTE[size_m];
}

ROM::~ROM()
{
    debugss(ssROM, INFO, "Destroying ROM\n");

    delete[] data_m;
    data_m = 0;
    size_m = 0;
}

ROM*
ROM::getROM(const char* filename,
            WORD        addr)
{
    std::ifstream     file;
    unsigned long int fileSize;
    BYTE*             buf;
    ROM*              rom = nullptr;

    file.open(filename, std::ios::binary);

    if (!file.is_open())
    {
        debugss(ssROM, ERROR, "ROM image \"%s\" cannot be opened\n", filename);
        return nullptr;
    }

    file.seekg(0, std::ios::end);
    fileSize = file.tellg();
    file.seekg(0, std::ios::beg);

    if (fileSize != 2048 && fileSize != 4096)
    {
        debugss(ssROM, ERROR, "ROM image \"%s\" has invalid size %d\n", filename, fileSize);
        return nullptr;
    }

    buf = new BYTE[fileSize];
    file.read((char*) buf, fileSize);
    file.close();

    rom = new ROM(fileSize);
    rom->setBaseAddress(addr);
    rom->initialize(buf, fileSize);
    return rom;
}

void
ROM::setBaseAddress(WORD adr) {
    base_m = adr;
}

void
ROM::initialize(BYTE* block,
                WORD  size)
{
    debugss(ssROM, INFO, "size(%d)\n", size);

    if (size > size_m)
    {
        // error attempting to store too much
        debugss(ssROM, ERROR, "size to big size(%d) size_m(%d)\n", size, size_m);
        size = size_m;
    }

    for (int i = 0; i < size; i++)
    {
        debugss(ssROM, VERBOSE, "block[%d] = %d\n", i, block[i]);
        data_m[i] = block[i];
    }
}


void
ROM::writeByte(WORD addr,
               BYTE val)
{
    // can't write to ROM.
    /// \todo update to set the RAM.
    debugss(ssROM, INFO, "Attempting to write to ROM [%d] = %d\n", addr, val);
    return;
}

BYTE
ROM::readByte(WORD addr)
{
    BYTE val    = 0;
    WORD offset = addr - base_m;

    if ((addr < base_m) || (offset >= size_m))
    {
        debugss(ssROM, ERROR, "Invalid address: %d (base: %d/size: %d)\n", addr, base_m, size_m);
        assert((addr >= base_m) && (offset < size_m));

    }

    else
    {
        val = data_m[offset];
    }

    debugss(ssROM, ALL, "addr[%d] = %d\n", addr, val);
    return (val);
}

WORD
ROM::getBase() {
    return base_m;
}
WORD
ROM::getSize() {
    return size_m;
}
BYTE*
ROM::getImage() {
    return data_m;
}
