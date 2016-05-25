/// \file DiskController.cpp
///
/// A wrapper for an IODevice that allows it to be distinguished from other,
/// non-storage, I/O Devices. Requires some methods for handling mounted media.
///
/// \date Feb 8, 2016
/// \author Douglas Miller
///

#include "DiskController.h"

/// \cond
#include <sstream>
/// \endcond



DiskController::DiskController(BYTE base,
                               BYTE numPorts): IODevice(base, numPorts)
{

}

DiskController::~DiskController()
{

}

std::string
DiskController::getDriveName(int index)
{
    std::ostringstream name;
    name << getDeviceName() << '-' << (index + 1);
    return name.str();
}
