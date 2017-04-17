///
///  \file DiskSide.h
///
///  \author Mark Garlanger
///
///  \date June 27, 2016
///

#ifndef DISK_SIDE_H_
#define DISK_SIDE_H_

#include "h89Types.h"

/// \cond
#include <vector>
#include <memory>
/// \endcond


class Track;
class Sector;

class DiskSide
{
  public:
    DiskSide(BYTE sideNum);
    ~DiskSide();

    bool addTrack(std::shared_ptr<Track> track);
    std::shared_ptr<Sector> findSector(BYTE trackNum, BYTE sectorNum);

  private:
    BYTE                                   sideNum_m;
    std::vector <std::shared_ptr<Track> >  trackData_m;

};


#endif // DISK_SIDE_H_
