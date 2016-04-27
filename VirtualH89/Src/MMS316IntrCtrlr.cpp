///
/// \name MMS316IntrCtrlr.cpp
///
/// Implementation of the H89 interrupt controller circuits, when augmented by the
/// MMS 77316 floppy disk controller. Used only if the MMS77316 is installed.
///
/// \date Feb 8, 2016
/// \author Douglas Miller
///

#include "MMS316IntrCtrlr.h"

#include "logger.h"
#include "cpu.h"

MMS316IntrCtrlr::MMS316IntrCtrlr(CPU* cpu): InterruptController(cpu),
                                            intrqRaised_m(false),
                                            drqRaised_m(false)
{
    debugss(ssMMS77316, INFO, "\n");
}

MMS316IntrCtrlr::~MMS316IntrCtrlr()
{

}

void
MMS316IntrCtrlr::setINTLine()
{
    if (intLevel_m != 2 && intLevel_m != 0)
    {
        debugss(ssInterruptController, VERBOSE, "intLevel: %d  intrqRaised_m: %d drqRaised: %d\n",
                intLevel_m, intrqRaised_m, drqRaised_m);
    }

    if (intLevel_m != 0 || intrqRaised_m || drqRaised_m)
    {
        cpu_m->raiseINT();
        cpu_m->continueRunning();
    }
    else
    {
        cpu_m->lowerINT();
    }
}

// reading instructions for interrupts
BYTE
MMS316IntrCtrlr::readDataBus()
{
    BYTE opCode = 0;

    if (intrqRaised_m)
    {
        // RST 30H
        opCode = 0xf7;
    }
    else if (drqRaised_m)
    {
        // EI
        opCode = 0xfb;
    }
    else
    {
        opCode = InterruptController::readDataBus();
    }

    debugss(ssMMS77316, VERBOSE, "readDataBus in interrupt... returning %02x\n", opCode);
    return opCode;
}



void
MMS316IntrCtrlr::setDrq(bool raise)
{
    debugss(ssMMS77316, VERBOSE, "%d\n", raise);
    drqRaised_m = raise;
    setINTLine();
}

void
MMS316IntrCtrlr::setIntrq(bool raise)
{
    debugss(ssMMS77316, VERBOSE, "%d\n", raise);
    intrqRaised_m = raise;
    setINTLine();

}
