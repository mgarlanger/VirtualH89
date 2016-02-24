/// \file DiskDrive.cpp
///
/// \date May 2, 2009
/// \author Mark Garlanger
///

#include "h-17-1.h"

#include "logger.h"
#include "FloppyDisk.h"


H_17_1::H_17_1()
{
    numTracks_m = maxTracks_c;
}

H_17_1::~H_17_1()
{
}


void
H_17_1::insertDisk(FloppyDisk* disk)
{
    disk_m = disk;

    // check for a Null disk insertion.
    if (disk_m)
    {
        disk_m->setMaxTrack(numTracks_m);
    }
}

void
H_17_1::getControlInfo(unsigned int pos, bool& hole, bool& trackZero, bool& writeProtect)
{
    // Track info comes from the drive, the hole and write-protect is determined
    // by the actual disk
    debugss(ssH17_1, INFO, "%s - pos: %d\n", __FUNCTION__, pos);

    trackZero = (track_m == 0);

    if (disk_m)
    {
        disk_m->getControlInfo(pos, hole, writeProtect);
    }

    else
    {
        debugss(ssH17_1, INFO, "%s no disk_m\n", __FUNCTION__);
        hole         = true;
        writeProtect = false;
    }
}

void
H_17_1::step(bool direction)
{
    if (direction)
    {
        if (track_m < 39)
        {
            ++track_m;
        }

        debugss(ssH17_1, INFO, "%s - in(up) (%d)\n", __FUNCTION__, track_m);
    }

    else
    {
        if (track_m)
        {
            --track_m;
        }

        debugss(ssH17_1, INFO, "%s - out(down) (%d)\n", __FUNCTION__, track_m);
    }
}

void
H_17_1::selectSide(BYTE side)
{
    // Since the H-17-1 is single-sided, this is a NOP.
}

BYTE
H_17_1::readData(unsigned int pos)
{
    BYTE data = 0;

    if ((disk_m) && (disk_m->readData(head_c, track_m, pos, data)))
    {
        debugss(ssH17_1, INFO, "%s: read passed - pos(%d) data(%d)\n", __FUNCTION__, pos, data);
    }

    else
    {
        debugss(ssH17_1, WARNING, "%s: read failed - pos(%d)\n", __FUNCTION__, pos);
    }

    return data;
}

void
H_17_1::writeData(unsigned int pos, BYTE data)
{
    if ((disk_m) && (!disk_m->writeData(head_c, track_m, pos, data)))
    {
        debugss(ssH17_1, WARNING, "%s: pos(%d)\n", __FUNCTION__, pos);
    }
}

BYTE
H_17_1::readSectorData(BYTE sector, unsigned int pos)
{
    BYTE data = 0;

    if ((disk_m) && (disk_m->readSectorData(0, track_m, sector, pos, data)))
    {
        debugss(ssH17_1, INFO, "%s: read passed - pos(%d) data(%d)\n", __FUNCTION__, pos, data);
    }

    else
    {
        debugss(ssH17_1, WARNING, "%s: read failed - pos(%d)\n", __FUNCTION__, pos);
    }

    return data;
}
