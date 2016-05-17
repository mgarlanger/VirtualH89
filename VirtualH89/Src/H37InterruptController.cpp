///
///  \file H37InterruptController.cpp
///
///  \author Mark Garlanger
///  \date  Mar. 17, 2016
///

#include "H37InterruptController.h"

#include "cpu.h"

#include "logger.h"


H37InterruptController::H37InterruptController(CPU* cpu): InterruptController(cpu),
                                                          intrqRaised_m(false),
                                                          drqRaised_m(false),
                                                          interruptsBlocked_m(false)
{
    debugss(ssH37InterruptController, INFO, "Entering\n");
}


H37InterruptController::~H37InterruptController()
{
    debugss(ssH37InterruptController, INFO, "Entering\n");
}


void
H37InterruptController::setINTLine()
{
    // debugss(ssH37InterruptController, INFO, "intLevel: %d intrqRaised_m: %d drqRaised: %d\n",
    //        intLevel_m, intrqRaised_m, drqRaised_m);

    if ((!interruptsBlocked_m && intLevel_m != 0) || intrqRaised_m || drqRaised_m)
    {
        cpu_m->raiseINT();
        cpu_m->continueRunning();
    }
    else
    {
        cpu_m->lowerINT();
    }
}

/// reading instructions for interrupts
/// this models the real hardware, in which the circuit generated the right
/// bits to be the desired instruction
BYTE
H37InterruptController::readDataBus()
{
    BYTE opCode = 0;

    // debugss(ssH37InterruptController, INFO, "intLevel: %d intrqRaised_m: %d drqRaised: %d\n",
    //         intLevel_m, intrqRaised_m, drqRaised_m);

    if (intrqRaised_m)
    {
        debugss(ssH37InterruptController, INFO, "intrq set\n");

        // RST 20H
        opCode = 0xe7;
    }
    else if (drqRaised_m)
    {
        // EI
        debugss(ssH37InterruptController, ALL, "drq set\n");
        opCode = 0xfb;
    }
    else if (!interruptsBlocked_m)
    {
        opCode = InterruptController::readDataBus();
    }
    else
    {
        debugss(ssH37InterruptController, ERROR, "interruptsBlocked_m, requesting instruction\n");
        opCode = InterruptController::readDataBus();
    }

    debugss(ssH37InterruptController, ALL, "opCode: %d\n", opCode);
    return opCode;
}

void
H37InterruptController::setIntrq(bool raise)
{
    debugss(ssH37InterruptController, VERBOSE, "Entering: %d\n", raise);
    intrqRaised_m = raise;
    setINTLine();
}

void
H37InterruptController::setDrq(bool raise)
{
    debugss(ssH37InterruptController, VERBOSE, "Entering: %d\n", raise);
    drqRaised_m = raise;
    setINTLine();
}

void
H37InterruptController::blockInterrupts(bool block)
{
    debugss(ssH37InterruptController, INFO, "Entering: %d\n", block);

    interruptsBlocked_m = block;
    setINTLine();
}
