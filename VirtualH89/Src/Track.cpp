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

using namespace std;

Track::Track(BYTE sideNum,
             BYTE trackNum): sideNum_m(sideNum),
                             trackNum_m(trackNum),
                             density_m(density_Unknown),
                             dataRate_m(dr_Unknown),
                             formattingMode_m(fs_none)
{

}

Track::~Track()
{
}

void
Track::startFormat()
{
    sectors_m.clear();
    formattingMode_m = fs_waitingForIndex;

}

BYTE
Track::getTrackNumber()
{

    return trackNum_m;
}

BYTE
Track::getSideNumber()
{

    return sideNum_m;
}

bool
Track::addSector(shared_ptr<Sector> sector)
{
    sectors_m.push_back(sector);

    return false;
}

void
Track::dump()
{
    debugss(ssFloppyDisk, INFO, "Dumping track - head: %d track: %d\n", sideNum_m,
            trackNum_m);


    for (shared_ptr<Sector> sector : sectors_m)
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


shared_ptr<Sector>
Track::findSector(BYTE sectorNum)
{
    for (shared_ptr<Sector> sector : sectors_m)
    {
        if (sector->getSectorNum() == sectorNum)
        {
            debugss(ssFloppyDisk, INFO, "found\n");

            return sector;
        }
    }

    return nullptr;
}
