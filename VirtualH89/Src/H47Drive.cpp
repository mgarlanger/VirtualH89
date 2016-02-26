///
/// \name H47Disk.cpp
///
///
/// \date Jul 27, 2013
/// \author Mark Garlanger
///

#include "H47Drive.h"

#include "logger.h"

H47Drive::H47Drive(): tracks_m(maxTracks_c),
                      sectors_m(maxSectorsPerTrack_c),
                      bytes_m(maxBytesPerSector_c),
                      track_m(0),
                      sector_m(1),
                      byte_m(0)
{
    // TODO Auto-generated constructor stub


}

H47Drive::~H47Drive()
{
    // TODO Auto-generated destructor stub
}

void
H47Drive::insertDisk(FloppyDisk* disk)
{
    disk_m = disk;

}

void
H47Drive::getControlInfo(unsigned long pos, bool& hole, bool& trackZero, bool& writeProtect)
{

}

void
H47Drive::selectSide(BYTE side)
{

}

void
H47Drive::step(bool direction)
{
    if (direction)
    {
        if (track_m < (maxTracks_c - 1))
        {
            ++track_m;
        }

        debugss(ssH47Drive, INFO, "%s - in(up) (%d)\n", __FUNCTION__, track_m);
    }
    else
    {
        if (track_m)
        {
            --track_m;
        }

        debugss(ssH47Drive, INFO, "%s - out(down) (%d)\n", __FUNCTION__, track_m);
    }

}

BYTE
H47Drive::readData(unsigned long pos)
{

    return 0;
}

void
H47Drive::writeData(unsigned long pos, BYTE data)
{

}

BYTE
H47Drive::readSectorData(BYTE sector, unsigned long pos)
{

    return 0;
}
