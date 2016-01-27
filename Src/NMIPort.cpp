///
/// \name NMIPort.cpp
///
/// \date Jul 15, 2012
/// \author Mark Garlanger
///

#include "NMIPort.h"
#include "H89.h"
#include "logger.h"

NMIPort::NMIPort(BYTE base, BYTE size): IODevice(base, size)
{

}
NMIPort::~NMIPort()
{

}

BYTE NMIPort::in(BYTE addr)
{
    debugss(ssIO, INFO, "%s: In: %d\n", __FUNCTION__, addr);

	h89.raiseNMI();
	/// \todo Determine the right value to return.

	return(0xff);
}

void NMIPort::out(BYTE addr, BYTE val)
{
    debugss(ssIO, INFO, "%s: Out: %d\n", __FUNCTION__, addr);
	h89.raiseNMI();
}
