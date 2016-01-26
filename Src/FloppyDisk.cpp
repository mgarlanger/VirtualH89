/// \file FloppyDisk.cpp
///
/// \date May 19, 2009
/// \author Mark Garlanger
///

#include "FloppyDisk.h"


FloppyDisk::FloppyDisk(): writeProtect_m(false),
                          maxTrack_m(0),
                          maxPos_m(0)
{

}

FloppyDisk::FloppyDisk(const char *name): writeProtect_m(false),
                                          maxTrack_m(0),
                                          maxPos_m(0)
{

}

FloppyDisk::~FloppyDisk()
{

}

void FloppyDisk::setWriteProtect(bool value)
{
    writeProtect_m = value;
}

bool FloppyDisk::checkWriteProtect(void)
{
    return(writeProtect_m);
}

void FloppyDisk::setMaxTrack(BYTE maxTrack)
{
    maxTrack_m = maxTrack;
}

void FloppyDisk::setMaxPosition(unsigned int maxPosition)
{
    maxPos_m = maxPosition;
}
