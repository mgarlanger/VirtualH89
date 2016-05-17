/// \file GenericFloppyDisk.h
///
/// \brief virtual Floppy Disk interface.
///
/// \date Feb 2, 2016
///
/// \author Douglas Miller
///

#ifndef GENERICFLOPPYDISK_H_
#define GENERICFLOPPYDISK_H_


#include "h89Types.h"

#include <string>
#include <memory>
#include <vector>

class Sector;
class Track;

/// \class GenericFloppyDisk
///
/// \brief A virtual floppy disk
///
class GenericFloppyDisk
{
  public:
    GenericFloppyDisk();
    virtual ~GenericFloppyDisk();

    bool checkWriteProtect()
    {
        return writeProtect_m;
    }
    bool doubleDensity()
    {
        return doubleDensity_m;
    }

    virtual bool readData(BYTE track,
                          BYTE side,
                          BYTE sector,
                          int  inSector,
                          int& data) = 0;
    virtual bool writeData(BYTE track,
                           BYTE side,
                           BYTE sector,
                           int  inSector,
                           BYTE data,
                           bool dataReady,
                           int& result)    = 0;
    virtual BYTE getMaxSectors(BYTE side,
                               BYTE track) = 0;

    virtual bool isReady()                 = 0;
    virtual void eject(const char* name)   = 0;
    virtual void dump(void)                = 0;
    virtual std::string getMediaName()     = 0;

    enum FileType_t
    {
        IMD,
        TD0,
        Unknown
    };
    enum DiskSize_t
    {
        FiveAndQuarterInch,
        EightInch
    };
    static std::shared_ptr<GenericFloppyDisk> loadDiskImage(std::vector<std::string> argv);

    void setDriveType(BYTE numTracks);

  private:

  protected:

    static FileType_t determineFileType(std::string filename);

    bool writeProtect_m;
    bool doubleDensity_m;
    long trackLen_m;
    int  numTracks_m;
    int  numSectors_m;
    int  numSides_m;
    int  secSize_m;
    int  mediaSize_m;

    ///
/*
    const char*               imageName_m;
    static const unsigned int maxHeads_c = 2;

    std::vector <Track*>      tracks_m[maxHeads_c];

    std::shared_ptr<Sector>   curSector_m;
    int                       dataPos_m;
    int                       sectorLength_m;
    BYTE                      secLenCode_m;
    bool                      ready_m;
 */
    bool hypoTrack_m;  // ST media in DT drive
    bool hyperTrack_m; // DT media in ST drive

};

#endif // GENERICFLOPPYDISK_H_
