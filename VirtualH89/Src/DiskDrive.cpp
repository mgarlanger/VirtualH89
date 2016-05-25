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

/// \cond
#include <memory>
/// \endcond

using namespace std;

DiskDrive::DiskDrive(BYTE tracks): disk_m(0),
                                   headLoaded_m(false),
                                   numTracks_m(tracks)
{

}

DiskDrive::~DiskDrive()
{

}



shared_ptr<DiskDrive>
DiskDrive::getInstance(string type)
{
    shared_ptr<DiskDrive> drive = nullptr;

    if ((type.compare("FDD_5_25_SS_ST") == 0) || (type.compare("H17_1") == 0))
    {
        debugss(ssDiskDrive, INFO, "allocating h17-1 drive\n");
        drive = make_shared<H_17_1>();
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
        debugss(ssDiskDrive, INFO, "allocating h17-4 drive\n");
        drive = make_shared<H_17_4>();
    }
    else
    {
        debugss(ssDiskDrive, ERROR, "unable to allocate drive\n");
    }
    return drive;
}

void
DiskDrive::insertDisk(shared_ptr<FloppyDisk> disk)
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

    disk_m = nullptr;
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
