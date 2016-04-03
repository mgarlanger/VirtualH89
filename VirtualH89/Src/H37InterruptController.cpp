///
///  \file H37InterruptController.cpp
///
///  \author Mark Garlanger
///  \date  Mar. 17, 2016
///

#include "H37InterruptController.h"
#include "logger.h"

#include "cpu.h"


H37InterruptController::H37InterruptController(CPU* cpu): InterruptController(cpu),
                                                          drqRaised_m(false)
{
    debugss(ssH37InterruptController, INFO, "%s: Entering\n", __FUNCTION__);

}


H37InterruptController::~H37InterruptController()
{
    debugss(ssH37InterruptController, INFO, "%s: Entering\n", __FUNCTION__);

}


void
H37InterruptController::setINTLine()
{
    if (intLevel_m != 2 && intLevel_m != 0)
    {
        debugss(ssH37InterruptController, VERBOSE, "%s: intLevel: %d  drqRaised: %d\n",
                __FUNCTION__, intLevel_m, drqRaised_m);
    }
    if (intLevel_m != 0 || drqRaised_m)
    {
        cpu_m->raiseINT();
    }
    else
    {
        cpu_m->lowerINT();
    }
}

// reading instructions for interrupts
BYTE
H37InterruptController::readDataBus()
{
    BYTE op = 0;
    // debugss(ssH37InterruptController, VERBOSE, "%s: Entering\n", __FUNCTION__);

    if (drqRaised_m)
    {
        op = 0xfb; // EI
    }
    else
    {
        op = InterruptController::readDataBus();
    }

    if (op != 207)
    {
        debugss(ssH37InterruptController, VERBOSE, "%s: op: %d\n", __FUNCTION__, op);
    }
    return op;
}


void
H37InterruptController::drq(bool raise)
{
    debugss(ssH37InterruptController, VERBOSE, "%s: Entering: %d\n", __FUNCTION__, raise);
    drqRaised_m = raise;
    setINTLine();
}
