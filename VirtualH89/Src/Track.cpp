///
/// \name Track.cpp
///
///
/// \date Jun 21, 2013
/// \author Mark Garlanger
///

#include "Track.h"
#include "Sector.h"

#include "logger.h"

Track::Track(BYTE sideNum,
             BYTE trackNum): sideNum_m(sideNum),
                             trackNum_m(trackNum),
                             density_m(density_Unknown),
                             dataRate_m(dr_Unknown)
{

}

Track::~Track()
{
//    if (sectors_m)
//    {
//        delete[] sectors_m;
//    }
}

bool
Track::addSector(Sector* sector)
{
    sectors_m.push_back(sector);

    return false;
}

void
Track::dump()
{
    debugss(ssFloppyDisk, INFO, "Dumping track - head: %d track: %d\n", sideNum_m,
            trackNum_m);


    for (Sector* sector : sectors_m)
    {
        debugss(ssFloppyDisk, INFO, "  Sector: %d\n", sector->getSectorNum());
        sector->dump();
    }
}

void
Track::setDensity(Density density)
{
    density_m = density;
}

void
Track::setDataRate(DataRate datarate)
{
    dataRate_m = datarate;
}

bool
Track::readSectorData(BYTE  sectorNum,
                      WORD  pos,
                      BYTE& data)
{
    debugss(ssFloppyDisk, INFO, "sector: %d pos: %d\n", sectorNum, pos);

    for (Sector* sector : sectors_m)
    {
        if (sector->getSectorNum() == sectorNum)
        {
            debugss(ssFloppyDisk, ALL, "found\n");

            return sector->readData(pos, data);
        }
    }

    debugss(ssFloppyDisk, INFO, "Not found\n");

    return false;

}

Sector*
Track::findSector(BYTE sectorNum)
{
    for (Sector* sector : sectors_m)
    {
        if (sector->getSectorNum() == sectorNum)
        {
            debugss(ssFloppyDisk, INFO, "found\n");

            return sector;
        }
    }

    return nullptr;
}

BYTE
Track::getMaxSectors()
{
    // todo see if this should look through and find the highest sector number from all sectors.
    // that is properly the right thing to do.
    return sectors_m.size() & 0xff;
}
