/// \file AddressBus.cpp
///
///  \date Mar 7, 2009
///  \author Mark Garlanger
///

#include "AddressBus.h"

#include "logger.h"
#include "MemoryDecoder.h"
#include "InterruptController.h"

AddressBus::AddressBus(InterruptController* ic): ic_m(ic), mem(NULL)
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

    // Could do this entirely inside MemoryLayout, but this way we have
    // more options for direct access to all memory.
    int bnk = mem->getCurrentBank();

    debugss(ssAddressBus, ALL, "addr(%d), bank(%d)\n", addr, bnk);

    return mem->readByte(bnk, addr);
}

void
AddressBus::writeByte(WORD addr,
                      BYTE val)
{

    // Could do this entirel inside MemoryLayout, but this way we have
    // more options for direct access to all memory.
    int bnk = mem->getCurrentBank();

    debugss(ssAddressBus, ALL, "addr(%d) = %d, bank(%d)\n", addr, val, bnk);

    mem->writeByte(bnk, addr, val);
}



void
AddressBus::installMemory(MemoryDecoder* memory)
{
    mem = memory;
}

void
AddressBus::clearMemory(BYTE data)
{
    debugss(ssAddressBus, INFO, "data(%d)\n", data);

    // TODO: implement this if needed - it is never used right now.
}

InterruptController*
AddressBus::getIntrCtrlr()
{
    return ic_m;
}

void
AddressBus::setIntrCtrlr(InterruptController* ic)
{
    ic_m = ic;
}

void
AddressBus::reset()
{
    if (mem != NULL)
    {
        mem->reset();
    }
}
