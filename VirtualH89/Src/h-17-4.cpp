///
/// \file h-17-4.cpp
///
/// \date Jul 21, 2012
/// \author Mark Garlanger
///

#include "h-17-4.h"

#include "logger.h"
#include "FloppyDisk.h"


H_17_4::H_17_4(): DiskDrive(maxTracks_c),
                  side_m(0),
                  track_m(0)
{

}

H_17_4::~H_17_4()
{

}


void
H_17_4::getControlInfo(unsigned long pos,
                       bool&         hole,
                       bool&         trackZero,
                       bool&         writeProtect)
{
    debugss(ssH17_4, INFO, "pos: %ld\n", pos);

    trackZero = (track_m == 0);

    if (disk_m)
    {
        disk_m->getControlInfo(pos, hole, writeProtect);
    }
    else
    {
        debugss(ssH17_4, INFO, "no disk_m\n");
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

        debugss(ssH17_4, WARNING, "in(up) (%d)\n", track_m);
    }
    else
    {
        if (track_m)
        {
            --track_m;
        }

        debugss(ssH17_4, WARNING, "out(down) (%d)\n", track_m);
    }
}

void
H_17_4::selectSide(BYTE side)
{
    if (side == side_m)
    {
        return;
    }

    debugss(ssH17_4, WARNING, "%d\n", side);

    if ((side == 0) || (side == 1))
    {
        side_m = side;
    }
    else
    {
        debugss(ssH17_4, ERROR, "Invalid Side: %d\n", side);
    }
}


BYTE
H_17_4::readData(unsigned long pos)
{
    BYTE data = 0;

    if ((disk_m) && (disk_m->readData(side_m, track_m, pos, data)))
    {
        debugss(ssH17_4, INFO, "read passed - pos(%lu) data(%d)\n", pos, data);
    }
    else
    {
        debugss(ssH17_4, WARNING, "read failed - pos(%lu)\n", pos);
    }

    return data;
}

void
H_17_4::writeData(unsigned long pos,
                  BYTE          data)
{
    debugss(ssH17_4, INFO, "pos(%lu) data(%d)\n", pos, data);

    if ((disk_m) && (!disk_m->writeData(side_m, track_m, pos, data)))
    {
        debugss(ssH17_4, WARNING, "pos(%lu)\n", pos);
    }
}

BYTE
H_17_4::readSectorData(BYTE          sector,
                       unsigned long pos)
{
    BYTE data = 0;

    if ((disk_m) && (disk_m->readSectorData(side_m, track_m, sector, pos, data)))
    {
        debugss(ssH17_4, INFO, "read passed - pos(%lu) data(%d)\n", pos, data);
    }
    else
    {
        debugss(ssH17_4, WARNING, "read failed - pos(%lu)\n", pos);
    }

    return data;

}
