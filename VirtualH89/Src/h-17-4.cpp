///
/// \file h-17-4.cpp
///
/// \date Jul 21, 2012
/// \author Mark Garlanger
///

#include "h-17-4.h"

#include "logger.h"
#include "FloppyDisk.h"


H_17_4::H_17_4(): side_m(0),
                  track_m(0)
{
    numTracks_m = maxTracks_c;

}

H_17_4::~H_17_4()
{

}

void
H_17_4::insertDisk(FloppyDisk* disk)
{
    disk_m = disk;

    // check for a Null disk insertion.
    if (disk_m)
    {
        disk_m->setMaxTrack(numTracks_m);
    }
}

void
H_17_4::getControlInfo(unsigned long pos,
                       bool&         hole,
                       bool&         trackZero,
                       bool&         writeProtect)
{
    debugss(ssH17_4, INFO, "%s - pos: %ld\n", __FUNCTION__, pos);

    trackZero = (track_m == 0);

    if (disk_m)
    {
        disk_m->getControlInfo(pos, hole, writeProtect);
    }
    else
    {
        debugss(ssH17_4, INFO, "%s no disk_m\n", __FUNCTION__);
        hole         = true;
        writeProtect = false;
    }
}

void
H_17_4::step(bool direction)
{
    if (direction)
    {
        if (track_m < 79)
        {
            ++track_m;
        }

        debugss(ssH17_4, WARNING, "%s - in(up) (%d)\n", __FUNCTION__, track_m);
    }
    else
    {
        if (track_m)
        {
            --track_m;
        }

        debugss(ssH17_4, WARNING, "%s - out(down) (%d)\n", __FUNCTION__, track_m);
    }
}

void
H_17_4::selectSide(BYTE side)
{
    if (side == side_m)
    {
        return;
    }

    debugss(ssH17_4, WARNING, "%s: %d\n", __FUNCTION__, side);

    if ((side == 0) || (side == 1))
    {
        side_m = side;
    }
    else
    {
        debugss(ssH17, ERROR, "%s: Invalid Side: %d\n", __FUNCTION__, side);
    }
}


BYTE
H_17_4::readData(unsigned long pos)
{
    BYTE data = 0;

    if ((disk_m) && (disk_m->readData(side_m, track_m, pos, data)))
    {
        debugss(ssH17_4, INFO, "%s: read passed - pos(%lu) data(%d)\n", __FUNCTION__, pos, data);
    }
    else
    {
        debugss(ssH17_4, WARNING, "%s: read failed - pos(%lu)\n", __FUNCTION__, pos);
    }

    return data;
}

void
H_17_4::writeData(unsigned long pos, BYTE data)
{
    debugss(ssH17_4, INFO, "%s: pos(%lu) data(%d)\n", __FUNCTION__, pos, data);

    if ((disk_m) && (!disk_m->writeData(side_m, track_m, pos, data)))
    {
        debugss(ssH17_4, WARNING, "%s: pos(%lu)\n", __FUNCTION__, pos);
    }
}

BYTE
H_17_4::readSectorData(BYTE sector, unsigned long pos)
{
    BYTE data = 0;

    if ((disk_m) && (disk_m->readSectorData(side_m, track_m, sector, pos, data)))
    {
        debugss(ssH17_4, INFO, "%s: read passed - pos(%lu) data(%d)\n", __FUNCTION__, pos, data);
    }
    else
    {
        debugss(ssH17_4, WARNING, "%s: read failed - pos(%lu)\n", __FUNCTION__, pos);
    }

    return data;

}
