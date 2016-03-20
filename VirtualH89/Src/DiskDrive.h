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

#include <string>

#include "config.h"
#include "h89Types.h"

class FloppyDisk;

class DiskDrive
{
  public:
    static DiskDrive* getInstance(std::string type);

    DiskDrive(BYTE tracks = 40);
    virtual ~DiskDrive();

    virtual void insertDisk(FloppyDisk* disk);
    virtual void ejectDisk(const char* name);


    virtual void getControlInfo(unsigned long pos,
                                bool&         hole,
                                bool&         trackZero,
                                bool&         writeProtect)  = 0;

    virtual void selectSide(BYTE side)                       = 0;
    virtual void step(bool direction)                        = 0;

    virtual BYTE readData(unsigned long pos)                 = 0;
    virtual void writeData(unsigned long pos,
                           BYTE          data)               = 0;

    virtual BYTE readSectorData(BYTE          sector,
                                unsigned long pos)           = 0;

    virtual void loadHead();
    virtual void unLoadHead();
    virtual bool getHeadLoadStatus();


  protected:
    FloppyDisk* disk_m;

    bool        headLoaded_m;
    BYTE        numTracks_m;
    BYTE        track_m = 0;

};

#endif // DISKDRIVE_H_
