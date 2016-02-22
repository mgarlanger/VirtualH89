///
/// \file DiskDrive.h
///
/// \brief Base virtual Disk Drive
///
/// \date Jul 21, 2012
/// \author Mark Garlanger
///

#ifndef DISKDRIVE_H_
#define DISKDRIVE_H_

#include "config.h"
#include "h89Types.h"

class FloppyDisk;

class DiskDrive
{
  public:
    DiskDrive();
    virtual ~DiskDrive();

    virtual void insertDisk(FloppyDisk *disk) = 0;
    virtual void ejectDisk(const char *name);


    virtual void getControlInfo(unsigned int pos, bool& hole, bool& trackZero, bool& writeProtect) = 0;

    virtual void selectSide(BYTE side) = 0;
    virtual void step(bool direction) = 0;

    virtual BYTE readData(unsigned int pos) = 0;
    virtual void writeData(unsigned int pos, BYTE data) = 0;

    virtual BYTE readSectorData(BYTE sector, unsigned int pos) = 0;

    virtual void loadHead();
    virtual void unLoadHead();
    virtual bool getHeadLoadStatus();

  protected:
    FloppyDisk *disk_m;

    bool headLoaded_m;
    BYTE numTracks_m;
    BYTE track_m = 0;


};

#endif // DISKDRIVE_H_
