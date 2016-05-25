/// \file AddressBus.cpp
///
///  \date Mar 7, 2009
///  \author Mark Garlanger
///

#include "AddressBus.h"

#include "logger.h"
#include "MemoryDecoder.h"
#include "InterruptController.h"

using namespace std;

AddressBus::AddressBus(InterruptController* ic): ic_m(ic),
                                                 mem_m(nullptr)
{
    debugss(ssAddressBus, INFO, "\n");
}

AddressBus::~AddressBus()
{
    debugss(ssAddressBus, INFO, "\n");
}

BYTE
AddressBus::readByte(WORD addr,
                     bool interruptAck)
{

    if (interruptAck)
    {
        debugss(ssAddressBus, ALL, "interrupt\n");

        return ic_m->readDataBus();
    }

    debugss(ssAddressBus, ALL, "addr(%d)\n", addr);

    return mem_m->readByte(addr);
}

void
AddressBus::writeByte(WORD addr,
                      BYTE val)
{
    debugss(ssAddressBus, ALL, "addr(%d) = %d\n", addr, val);

    mem_m->writeByte(addr, val);
}

void
AddressBus::installMemory(shared_ptr<MemoryDecoder> memory)
{
    mem_m = memory;
}

void
AddressBus::reset()
{
    if (mem_m != nullptr)
    {
        mem_m->reset();
    }
}
