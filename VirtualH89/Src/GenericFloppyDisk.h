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

/// \cond
#include <string>
#include <memory>
#include <vector>
/// \endcond

class Sector;
class Track;

class GenericFloppyDisk;
typedef std::shared_ptr<GenericFloppyDisk> GenericFloppyDisk_ptr;

/// \class GenericFloppyDisk
///
/// \brief A virtual floppy disk
///
class GenericFloppyDisk
{
  public:
    GenericFloppyDisk();
    virtual ~GenericFloppyDisk();

    void setWriteProtect(bool value);

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
                           int& result) = 0;

    virtual BYTE getRealTrackNumber(BYTE track);
    virtual bool findSector(BYTE sideNum,
                            BYTE trackNum,
                            BYTE sectorNum)    = 0;

    virtual bool isReady()                     = 0;
    virtual void eject(const std::string name) = 0;
    virtual void dump(void)                    = 0;
    virtual std::string getMediaName();

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
    static GenericFloppyDisk_ptr loadDiskImage(std::vector<std::string> argv);

    void setDriveType(BYTE numTracks);

  private:

  protected:

    static FileType_t determineFileType(std::string filename);

    std::string imageName_m;
    bool        writeProtect_m;
    bool        doubleDensity_m;
    unsigned    trackLen_m;
    int         numTracks_m;
    int         numSectors_m;
    int         numSides_m;
    int         secSize_m;
    int         mediaSize_m;

    ///

    bool        hypoTrack_m;  // ST media in DT drive
    bool        hyperTrack_m; // DT media in ST drive

};


#endif // GENERICFLOPPYDISK_H_
