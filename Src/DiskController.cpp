/// \file DiskController.cpp
///
/// \date Feb 8, 2016
/// \author Douglas Miller
///

#include "DiskController.h"
#include <sstream>

DiskController::DiskController(BYTE base, BYTE numPorts): IODevice(base, numPorts)
{

}

DiskController::~DiskController()
{

}

std::string DiskController::getDriveName(int index)
{
    std::ostringstream name;
    name << getDeviceName() << '-' << (index + 1);
    return name.str();
}
