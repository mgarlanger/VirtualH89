///
/// \name NMIPort.cpp
///
/// \date Jul 15, 2012
/// \author Mark Garlanger
///

#include "NMIPort.h"

#include "logger.h"
#include "cpu.h"

NMIPort::NMIPort(CPU* cpu,
                 BYTE base,
                 BYTE size): IODevice(base, size),
                             cpu_m(cpu)
{

}
NMIPort::~NMIPort()
{

}

BYTE
NMIPort::in(BYTE addr)
{
    debugss(ssIO, INFO, "In: %d\n", addr);

    cpu_m->raiseNMI();
    /// \todo Determine the right value to return.

    return (0xff);
}

void
NMIPort::out(BYTE addr,
             BYTE val)
{
    debugss(ssIO, INFO, "Out: %d\n", addr);
    cpu_m->raiseNMI();
}
