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

#include "mms77316.h"
#include "logger.h"
#include "z80.h"

MMS316IntrCtrlr::MMS316IntrCtrlr(InterruptController* ic, MMS77316* m316): InterruptController(ic),
                                                                           m316_m(m316)
{
    debugss(ssMMS77316, VERBOSE, "MMS316IntrCtrlr(InterruptController *, MMS77316 *)\n");
}

MMS316IntrCtrlr::~MMS316IntrCtrlr()
{
}

void
MMS316IntrCtrlr::raiseInterrupt(BYTE level)
{
    debugss(ssMMS77316, VERBOSE, "MMS316IntrCtrlr::raiseInterrupt(%d)\n", level);
    InterruptController::raiseInterrupt(level);
}

void
MMS316IntrCtrlr::lowerInterrupt(BYTE level)
{
    debugss(ssMMS77316, VERBOSE, "MMS316IntrCtrlr::lowerInterrupt(%d)\n", level);
    InterruptController::lowerInterrupt(level);
}

// reading instructions for interrupts
BYTE
MMS316IntrCtrlr::readDataBus()
{
    BYTE opCode = 0;

    if (!m316_m->interResponder(opCode))
    {
        opCode = InterruptController::readDataBus();
    }

    debugss(ssMMS77316, VERBOSE, "readDataBus in interrupt... returning %02x\n", opCode);
    return opCode;
}
