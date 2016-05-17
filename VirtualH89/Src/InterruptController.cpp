///
/// \name InterruptController.cpp
///
///
/// \date Feb 7, 2016
/// \author Mark Garlanger
///

#include "InterruptController.h"

#include "logger.h"
#include "cpu.h"



InterruptController::InterruptController(CPU* cpu): intLevel_m(0),
                                                    cpu_m(cpu)
{
    debugss(ssInterruptController, INFO, "Entering\n");

}


InterruptController::~InterruptController()
{
    debugss(ssInterruptController, INFO, "Entering\n");

}


void
InterruptController::setINTLine()
{
    debugss(ssInterruptController, ALL, "\n");

    if (intLevel_m != 0)
    {
        cpu_m->raiseINT();
    }
    else
    {
        cpu_m->lowerINT();
    }
}


void
InterruptController::raiseInterrupt(BYTE level)
{
    debugss(ssInterruptController, ALL, "level(%d)\n", level);

    // verify level - only 0-7 are valid
    if ((level < 0) || (level > 7))
    {
        debugss(ssInterruptController, ERROR, "invalid level(%d)\n", level);
        return;
    }

    intLevel_m |= (1 << level);

    // raise interrupt line to cpu
    setINTLine();
}


void
InterruptController::lowerInterrupt(BYTE level)
{
    debugss(ssInterruptController, ALL, "level(%d)\n", level);

    // verify level - only 0-7 are valid
    if ((level < 0) || (level > 7))
    {
        // INT level out-of-range.
        debugss(ssInterruptController, ERROR, "invalid level(%d)\n", level);
        return;
    }

    intLevel_m &= ~(1 << level);

    // update interrupt line to cpu
    setINTLine();
}


// reading instructions for interrupts
BYTE
InterruptController::readDataBus()
{
    BYTE opCode = 0;


    if (intLevel_m & 0x80)
    {
        opCode = 0xff;
    }
    else if (intLevel_m & 0x40)
    {
        opCode = 0xf7;
    }
    else if (intLevel_m & 0x20)
    {
        opCode = 0xef;
    }
    else if (intLevel_m & 0x10)
    {
        opCode = 0xe7;
    }
    else if (intLevel_m & 0x08)
    {
        opCode = 0xdf;
    }
    else if (intLevel_m & 0x04)
    {
        opCode = 0xd7;
    }
    else if (intLevel_m & 0x02)
    {
        opCode = 0xcf;
    }
    else if (intLevel_m & 0x01)
    {
        opCode = 0xc7;
    }
    else
    {
        // invalid interrupt level.
        debugss(ssInterruptController, ERROR, "Invalid interrupt level: %d\n", intLevel_m);
    }

    debugss(ssInterruptController, ALL, "Interrupt Instruction %d\n", opCode);

    return opCode;
}

void
InterruptController::setDrq(bool raise)
{
    // shouldn't be called
    debugss(ssInterruptController, ERROR, "base called(%d)\n", raise);
}

void
InterruptController::setIntrq(bool raise)
{
    debugss(ssInterruptController, ERROR, "base called(%d)\n", raise);
}

void
InterruptController::blockInterrupts(bool block)
{
    debugss(ssInterruptController, ERROR, "base called(%d)\n", block);

}
