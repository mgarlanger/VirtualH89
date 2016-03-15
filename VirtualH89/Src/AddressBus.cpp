/// \file AddressBus.cpp
///
///  \date Mar 7, 2009
///  \author Mark Garlanger
///

#include "AddressBus.h"

#include "logger.h"
#include "MemoryDecoder.h"
#include "InterruptController.h"

AddressBus::AddressBus(InterruptController* ic):
    mem(NULL),
    ic_m(ic)
{
    debugss(ssAddressBus, INFO, "%s\n", __FUNCTION__);
}

AddressBus::~AddressBus()
{
    debugss(ssAddressBus, INFO, "%s\n", __FUNCTION__);
}

BYTE
AddressBus::readByte(WORD addr, bool interruptAck)
{

    if (interruptAck)
    {
        debugss(ssAddressBus, ALL, "%s interrupt\n", __FUNCTION__);

        return ic_m->readDataBus();
    }

    // Could do this entirely inside MemoryLayout, but this way we have
    // more options for direct access to all memory.
    int bnk = mem->getCurrentBank();

    debugss(ssAddressBus, ALL, "%s: addr(%d), bank(%d)\n", __FUNCTION__, addr, bnk);

    return mem->readByte(bnk, addr);
}

void
AddressBus::writeByte(WORD addr, BYTE val)
{

    // Could do this entirel inside MemoryLayout, but this way we have
    // more options for direct access to all memory.
    int bnk = mem->getCurrentBank();

    debugss(ssAddressBus, ALL, "%s: addr(%d) = %d, bank(%d)\n", __FUNCTION__,
            addr, val, bnk);

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
    debugss(ssAddressBus, INFO, "%s: data(%d)", __FUNCTION__, data);

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
