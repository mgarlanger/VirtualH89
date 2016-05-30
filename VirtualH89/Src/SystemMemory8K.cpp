///
/// \file SystemMemory8K.cpp
///
///
///  \author  Mark Garlanger
///  \date    May 8, 2016

#include "SystemMemory8K.h"

#include "ROM.h"

using namespace std;

SystemMemory8K::SystemMemory8K(): RAMemory8K(0x0000),
                                  maskRO(0),
                                  maskInstalled(0),
                                  RAM(nullptr)
{
}

void
SystemMemory8K::overlayRAM(shared_ptr<Memory8K> ram)
{
    RAM = ram;
}

void
SystemMemory8K::enableRAM(WORD base, WORD len)
{
    WORD adr = base & MemoryAddressMask_c;
    if (adr + len > sizeof(mem))
    {
        // error? or just trim?
        len = sizeof(mem) - adr;
    }
    int a = (adr >> 10) & 0x07;
    int n = a + (((len + 0x03ff) >> 10) & 0x07);
    // TODO: find more-elegant way
    for (; a < n; ++a)
    {
        maskInstalled |= (1 << a);
    }
}

void
SystemMemory8K::writeProtect(WORD adr, WORD len)
{
    int a = (adr >> 10) & 0x07;
    int n = a + (((len + 0x03ff) >> 10) & 0x07);
    // TODO: find more-elegant way
    for (; a < n && a < 8; ++a)
    {
        maskRO |= (1 << a);
    }
}

void
SystemMemory8K::writeEnable(WORD adr, WORD len)
{
    int a = (adr >> 10) & 0x07;
    int n = a + (((len + 0x03ff) >> 10) & 0x07);
    // TODO: find more-elegant way
    for (; a < n && a < 8; ++a)
    {
        maskRO &= ~(1 << a);
    }
}

void
SystemMemory8K::installROM(ROM* rom)
{
    WORD adr = rom->getBase() & MemoryAddressMask_c;
    int  len = rom->getSize();
    if (adr + len > sizeof(mem))
    {
        // error? or just trim?
        len = sizeof(mem) - adr;
    }
    memcpy(&mem[adr], rom->getImage(), len);
    int a = (adr >> 10) & 0x07;
    int n = a + (((len + 0x03ff) >> 10) & 0x07);
    // TODO: find more-elegant way
    for (; a < n; ++a)
    {
        maskRO        |= (1 << a);
        maskInstalled |= (1 << a);
    }
}
