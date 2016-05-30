///
/// \name Track.h
///
///
/// \date Jun 21, 2013
/// \author Mark Garlanger
///

#ifndef TRACK_H_
#define TRACK_H_

#include "h89Types.h"

/// \cond
#include <vector>
#include <memory>
/// \endcond

class Sector;

class Track
{
  public:
    enum DataRate
    {
        dr_Unknown,
        dr_250kbps,
        dr_300kbps,
        dr_500kbps
    };

    enum Density
    {
        density_Unknown,
        singleDensity,
        FM  = singleDensity,
        doubleDensity,
        MFM = doubleDensity
    };

  private:
    BYTE                                   sideNum_m;
    BYTE                                   trackNum_m;

    std::vector <std::shared_ptr<Sector> > sectors_m;

    Density                                density_m;
    DataRate                               dataRate_m;

  public:
    Track(BYTE sideNum,
          BYTE trackNum);
    virtual ~Track();

    bool addSector(std::shared_ptr<Sector> sector);

    void setDensity(Density density);
    void setDataRate(DataRate datarate);

    void dump();

    std::shared_ptr<Sector> findSector(BYTE sector);

    bool readSectorData(BYTE  sector,
                        WORD  pos,
                        BYTE& data);
    BYTE getMaxSectors();

};

#endif // TRACK_H_
