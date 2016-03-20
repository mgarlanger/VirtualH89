///
/// \file DiskDrive.cpp
///
/// \date Jul 21, 2012
/// \author Mark Garlanger
///

#include "DiskDrive.h"

#include "FloppyDisk.h"
#include "logger.h"

#include "h-17-1.h"
#include "h-17-4.h"

DiskDrive::DiskDrive(BYTE tracks): disk_m(0),
                                   headLoaded_m(false),
                                   numTracks_m(tracks)
{

}

DiskDrive::~DiskDrive()
{

}



DiskDrive*
DiskDrive::getInstance(std::string type)
{
    DiskDrive* drive = nullptr;

    if ((type.compare("FDD_5_25_SS_ST") == 0) || (type.compare("H17_1") == 0))
    {
        debugss(ssDiskDrive, ERROR, "%s - allocating h17-1 drive\n", __FUNCTION__);
        drive = new H_17_1();
    }
    // \todo support ss_dt & ds_st
//    else if (type.compare("FDD_5_25_SS_DT") == 0)
//    {
//        etype = FDD_5_25_SS_DT;
//    }
//    else if (type.compare("FDD_5_25_DS_ST") == 0)
//    {
//        etype = FDD_5_25_DS_ST;
//    }
    else if ((type.compare("FDD_5_25_DS_DT") == 0) || (type.compare("H17_4") == 0))
    {
        debugss(ssDiskDrive, ERROR, "%s - allocating h17-4 drive\n", __FUNCTION__);
        drive = new H_17_4();
    }
    else
    {
        debugss(ssDiskDrive, ERROR, "%s - unable to allocate drive\n", __FUNCTION__);
    }
    return drive;
}

void
DiskDrive::insertDisk(FloppyDisk* disk)
{
    disk_m = disk;

    // check for a Null disk insertion.
    if (disk_m)
    {
        disk_m->setMaxTrack(numTracks_m);
    }
}

void
DiskDrive::ejectDisk(const char* name)
{
    if (disk_m)
    {
        disk_m->eject(name);
    }

    disk_m = 0;
}

void
DiskDrive::loadHead()
{
    headLoaded_m = true;
}

void
DiskDrive::unLoadHead()
{
    headLoaded_m = false;
}

bool
DiskDrive::getHeadLoadStatus()
{
    return headLoaded_m;
}

#if 0
void
DiskDrive::step(bool direction)
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

#endif
