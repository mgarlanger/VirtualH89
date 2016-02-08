///
/// \name MMS316IntrCtrlr.cpp
///
///
/// \date Feb 8, 2016
/// \author Douglas Miller
///

#include "MMS316IntrCtrlr.h"

#include "logger.h"
#include "z80.h"

MMS316IntrCtrlr::MMS316IntrCtrlr(InterruptController *ic, MMS77316 *m316):
    InterruptController(ic->getCpu()),
    ic_m(ic),
    m316_m(m316)
{
}

MMS316IntrCtrlr::~MMS316IntrCtrlr()
{
}

void MMS316IntrCtrlr::raiseInterrupt(BYTE level)
{
    ic_m->raiseInterrupt(level);
}

void MMS316IntrCtrlr::lowerInterrupt(BYTE level)
{
    InterruptController::lowerInterrupt(level);
    ic_m->lowerInterrupt(level);
}

// reading instructions for interrupts
BYTE MMS316IntrCtrlr::readDataBus()
{
    BYTE opCode = 0;

    if (!m316_m->interResponder(opCode))
    {
        opCode = ic_m->readDataBus();
    }

    debugss(ssMMS77316, INFO, "readDataBus in interrupt... returning %02x\n", opCode);
    return opCode;
}
