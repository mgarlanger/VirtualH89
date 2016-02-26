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

#include <string>

#include "config.h"
#include "h89Types.h"

class DiskData;

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

    virtual bool readData(BYTE track, BYTE side, BYTE sector, int inSector, int& data) = 0;
    virtual bool writeData(BYTE track, BYTE side, BYTE sector, int inSector,
                           BYTE data, bool dataReady, int& result) = 0;
    virtual bool isReady() = 0;
    virtual void eject(const char *name) = 0;
    virtual void dump(void) = 0;
    virtual std::string getMediaName() = 0;

  private:

  protected:
    bool writeProtect_m;
    bool doubleDensity_m;
    long trackLen_m;
    int  numTracks_m;
    int  numSectors_m;
    int  numSides_m;
    int  secSize_m;
    int  mediaSize_m;

};

#endif // GENERICFLOPPYDISK_H_
