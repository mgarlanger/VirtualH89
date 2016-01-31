///
/// \file DiskDrive.cpp
///
/// \date Jul 21, 2012
/// \author Mark Garlanger
///

#include "DiskDrive.h"
#include "FloppyDisk.h"
#include "logger.h"


DiskDrive::DiskDrive(): disk_m(0),
    headLoaded_m(false),
    numTracks_m(40)
{

}

DiskDrive::~DiskDrive()
{

}

#if 0
void DiskDrive::insertDisk(FloppyDisk *disk)
{
    disk_m = disk;

    // check for a Null disk insertion.
    if (disk_m)
    {
        disk_m->setMaxTrack(numTracks_m);
    }
}
#endif

void DiskDrive::ejectDisk(const char *name)
{
    if (disk_m)
    {
        disk_m->eject(name);
    }

    disk_m = 0;
}

void DiskDrive::loadHead()
{
    headLoaded_m = true;
}

void DiskDrive::unLoadHead()
{
    headLoaded_m = false;
}

bool DiskDrive::getHeadLoadStatus()
{
    return headLoaded_m;
}

void DiskDrive::step(bool direction)
{
    if (direction)
    {
        if (track_m < 39)
        {
            ++track_m;
        }

        debugss(ssDiskDrive, INFO, "%s - in(up) (%d)\n", __FUNCTION__, track_m);
    }

    else
    {
        if (track_m)
        {
            --track_m;
        }

        debugss(ssDiskDrive, INFO, "%s - out(down) (%d)\n", __FUNCTION__, track_m);
    }
}

