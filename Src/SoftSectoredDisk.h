///
/// \name SoftSectoredDisk.h
///
///
/// \date Oct 2, 2012
/// \author Mark Garlanger
///

#ifndef SOFTSECTOREDDISK_H_
#define SOFTSECTOREDDISK_H_

#include "FloppyDisk.h"

#include <vector>

class Track;

class SoftSectoredDisk: public FloppyDisk
{
  public:
    enum DiskImageFormat
    {
        dif_Unknown,
        dif_RAW,
        dif_IMD,
        dif_TD0,
        dif_8RAW
    };

    SoftSectoredDisk(const char *name, DiskImageFormat format);
    SoftSectoredDisk();
    virtual ~SoftSectoredDisk();

    virtual bool readData(BYTE side, BYTE track, unsigned int pos, BYTE& data);
    virtual bool writeData(BYTE side, BYTE track, unsigned int pos, BYTE data);
    virtual void getControlInfo(unsigned int pos, bool& hole, bool& writeProtect);

    virtual bool readSectorData(BYTE side, BYTE track, BYTE sector, WORD pos, BYTE& data);
    virtual void eject(const char *name);

  private:
    static const unsigned int bytesPerTrack_c = 6400;

    static const unsigned int tracksPerSide_c = 80;
    static const unsigned int maxHeads_c      = 2;
    BYTE rawImage_m[maxHeads_c][tracksPerSide_c][bytesPerTrack_c];

    std::vector <Track *> tracks_m[maxHeads_c];
    BYTE  maxTrack_m;

    bool initialized_m    = false;
    BYTE numTracks_m      = 80;
    BYTE numHeads_m       = 2;
    BYTE numSectors_m     = 10;

    // move these to track, that is how it is encoded in IMD files.
//    DataRate dataRate_m;

    bool defaultHoleStatus(unsigned int pos);

    void determineDiskFormat(const char *name, DiskImageFormat& format);
    bool readTD0(const char *name);
    bool readIMD(const char *name);
    bool readRaw(const char *name);
    bool readRaw8(const char *name);

  protected:
    void dump();

};

#endif // SOFTSECTOREDDISK_H_
