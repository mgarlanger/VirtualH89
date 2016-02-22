/// \file SerialPortDevice.cpp
///
/// \date Apr 23, 2009
/// \author Mark Garlanger
///

#include "SerialPortDevice.h"

#include "INS8250.h"
#include <cstdio>

SerialPortDevice::SerialPortDevice(): port_m(0)
{

}

SerialPortDevice::~SerialPortDevice()
{

}

void SerialPortDevice::attachPort(INS8250 *port)
{
    port_m = port;
}

bool SerialPortDevice::sendReady()
{
    return port_m != NULL && port_m->receiveReady();
}

bool SerialPortDevice::sendData(BYTE data)
{
    if (port_m)
    {
        port_m->receiveData(data);
        return true;
    }

    printf("port_m is NULL\n");
    return false;
}

