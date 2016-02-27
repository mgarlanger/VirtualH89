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
    printf("Dumping track - head: %d track: %d\n", sideNum_m, trackNum_m);

    for (int sect = 0; sect < sectors_m.size(); sect++)
    {
        printf("  Sector: %d\n", sect);
        sectors_m[sect]->dump();
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
Track::readSectorData(BYTE  sector,
                      WORD  pos,
                      BYTE& data)
{
    debugss(ssFloppyDisk, INFO, "%s: sector: %d pos: %d\n", __FUNCTION__, sector, pos);

    for (int i = 0; i < sectors_m.size(); i++)
    {
        if (sectors_m[i]->getSectorNum() == sector)
        {
            debugss(ssFloppyDisk, INFO, "%s: found\n", __FUNCTION__);

            return sectors_m[i]->readData(pos, data);
        }
    }

    debugss(ssFloppyDisk, INFO, "%s: Not found\n", __FUNCTION__);

    return false;

}
