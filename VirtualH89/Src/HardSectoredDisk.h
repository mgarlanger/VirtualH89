///
/// \file HardSectoredDisk.h
///
///
/// \date Sep 9, 2012
/// \author Mark Garlanger
///

#ifndef HARDSECTOREDDISK_H_
#define HARDSECTOREDDISK_H_

#include "FloppyDisk.h"

class DiskData;

/// \class HardSectoredDisk
///
/// \brief Virtual hard-sectored disk, currently just supports SS/40-track disks.
///
/// This class implements a virtual hard-sectored disk. It models the entire track
/// such as the headers and gaps, not only the data portion.
///
class HardSectoredDisk: public FloppyDisk
{
  public:
    HardSectoredDisk(const char* name);
    HardSectoredDisk();
    virtual ~HardSectoredDisk();

    virtual bool readData(BYTE side, BYTE track, unsigned long pos, BYTE& data);
    virtual bool writeData(BYTE side, BYTE track, unsigned long pos, BYTE data);
    virtual void getControlInfo(unsigned long pos, bool& hole, bool& writeProtect);
    void dump();

    virtual bool readSectorData(BYTE side, BYTE track, BYTE sector, WORD pos,
                                BYTE& data);
    virtual void eject(const char* name);

  private:
    // Hard sectored disks on Heath, only support single density (FM encoding)
    static const unsigned int bytesPerTrack_c = 3200;

    /// \todo make the tracks (and sides) runtime set, so that double sided and 80 track
    /// hard-sectored disks can be used
    static const unsigned int maxHeads_c         = 2;
    static const unsigned int maxTracksPerSide_c = 80;

    BYTE                      rawImage_m[maxHeads_c][maxTracksPerSide_c][bytesPerTrack_c];

    bool                      initialized_m = false;
    unsigned int              tracks_m      = 80;
    unsigned int              sides_m       = 2;

    bool defaultHoleStatus(unsigned int pos);
};

#endif // HARDSECTOREDDISK_H_
