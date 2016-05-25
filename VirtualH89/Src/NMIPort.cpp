///
/// \name NMIPort.cpp
///
/// Some ports defined for the Heath H8 cause NMI to occur in the H89, this
/// allows the ROM program to handle those acesses as if the H89 is an H8.
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
