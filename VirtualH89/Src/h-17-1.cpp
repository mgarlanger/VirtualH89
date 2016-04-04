/// \file DiskDrive.cpp
///
/// \date May 2, 2009
/// \author Mark Garlanger
///

#include "h-17-1.h"

#include "logger.h"
#include "FloppyDisk.h"


H_17_1::H_17_1(): DiskDrive(maxTracks_c)
{

}

H_17_1::~H_17_1()
{
}


void
H_17_1::getControlInfo(unsigned long pos,
                       bool&         hole,
                       bool&         trackZero,
                       bool&         writeProtect)
{
    // Track info comes from the drive, the hole and write-protect is determined
    // by the actual disk
    debugss(ssH17_1, INFO, "pos: %ld\n", pos);

    trackZero = (track_m == 0);

    if (disk_m)
    {
        disk_m->getControlInfo(pos, hole, writeProtect);
    }
    else
    {
        debugss(ssH17_1, INFO, "no disk_m\n");
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

        debugss(ssH17_1, INFO, "in(up) (%d)\n", track_m);
    }
    else
    {
        if (track_m)
        {
            --track_m;
        }

        debugss(ssH17_1, INFO, "out(down) (%d)\n", track_m);
    }
}

void
H_17_1::selectSide(BYTE side)
{
    // Since the H-17-1 is single-sided, this is a NOP.
}

BYTE
H_17_1::readData(unsigned long pos)
{
    BYTE data = 0;

    if ((disk_m) && (disk_m->readData(head_c, track_m, pos, data)))
    {
        debugss(ssH17_1, INFO, "read passed - pos(%lu) data(%d)\n", pos, data);
    }
    else
    {
        debugss(ssH17_1, WARNING, "read failed - pos(%lu)\n", pos);
    }

    return data;
}

void
H_17_1::writeData(unsigned long pos,
                  BYTE          data)
{
    if ((disk_m) && (!disk_m->writeData(head_c, track_m, pos, data)))
    {
        debugss(ssH17_1, WARNING, "pos(%lu)\n", pos);
    }
}

BYTE
H_17_1::readSectorData(BYTE          sector,
                       unsigned long pos)
{
    BYTE data = 0;

    if ((disk_m) && (disk_m->readSectorData(0, track_m, sector, pos, data)))
    {
        debugss(ssH17_1, INFO, "read passed - pos(%lu) data(%d)\n", pos, data);
    }
    else
    {
        debugss(ssH17_1, WARNING, "read failed - pos(%lu)\n", pos);
    }

    return data;
}
