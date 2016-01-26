/// \file IODevice.cpp
///
/// \date Apr 20, 2009
/// \author Mark Garlanger
///

#include "IODevice.h"

IODevice::IODevice(BYTE base, BYTE numPorts): baseAddress_m(base),
                                              numPorts_m(numPorts)
{

}

IODevice::~IODevice()
{

}

BYTE IODevice::getBaseAddress(void)
{
    return(baseAddress_m);
}

BYTE IODevice::getNumPorts(void)
{
    return(numPorts_m);
}

bool IODevice::verifyPort(BYTE addr)
{
    return ((addr >= baseAddress_m) && (addr < (baseAddress_m + numPorts_m)));
}

BYTE IODevice::getPortOffset(BYTE addr)
{
	return(addr - baseAddress_m);
}
