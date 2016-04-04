///
/// \name InterruptController.cpp
///
///
/// \date Feb 7, 2016
/// \author Mark Garlanger
///

#include "InterruptController.h"

#include "logger.h"
#include "z80.h"


InterruptController::InterruptController(CPU* cpu): intLevel_m(0),
                                                    cpu_m(cpu)
{
    debugss(ssInterruptController, VERBOSE, "Entering\n");

}

InterruptController::InterruptController(InterruptController* ic):
    intLevel_m(ic->intLevel_m),
    cpu_m(ic->cpu_m)
{
    debugss(ssInterruptController, VERBOSE, "Entering\n");

}

InterruptController::~InterruptController()
{
    debugss(ssInterruptController, VERBOSE, "Entering\n");

}

void
InterruptController::raiseInterrupt(BYTE level)
{
    debugss(ssInterruptController, VERBOSE, "level(%d)\n", level);

    // verify level - only 0-7 are valid
    if ((level < 0) || (level > 7))
    {
        debugss(ssInterruptController, ERROR, "invalid level(%d)\n", level);
        return;
    }

    intLevel_m |= (1 << level);

    // raise interrupt line to Z80
    cpu_m->raiseINT();
}

void
InterruptController::lowerInterrupt(BYTE level)
{
    debugss(ssInterruptController, VERBOSE, "level(%d)\n", level);

    // verify level - only 0-7 are valid
    if ((level < 0) || (level > 7))
    {
        // INT level out-of-range.
        debugss(ssInterruptController, ERROR, "invalid level(%d)\n", level);
        return;
    }

    intLevel_m &= ~(1 << level);

    // ONLY clear int_type if no other interrupts are pending.
    if (!intLevel_m)
    {
        // lower interrupt line to Z80
        cpu_m->lowerINT();
    }
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

        // printf("Interrupt Instruction, bad interrupt level: %d\n", intLevel_m);
    }

    debugss(ssInterruptController, VERBOSE, "Interrupt Instruction %d\n", opCode);

    // printf("Interrupt Instruction: %d\n", opCode);
    return opCode;
}
