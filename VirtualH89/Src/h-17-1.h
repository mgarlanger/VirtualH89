/// \file h-17-1.h
///
/// \date  May 2, 2009
/// \author Mark Garlanger
///

#ifndef H_17_1_H_
#define H_17_1_H_


#include "DiskDrive.h"

///
/// \brief Virtual Single-Sided, 48 tpi Floppy Disk drive.
///
/// Implements a virtual disk drive. Single-Sided, 48 tpi.
/// When connected to the H17 (H-88-1) hard-sector controller, it supports
/// 102 k (40 tracks * 10 sectors/track * 256 bytes/sector)
/// When connected the the H37 (Z-89-37) soft-sector controller it supports
/// both single and double-density disks.
///
class H_17_1: public DiskDrive
{
  public:
    H_17_1();
    virtual ~H_17_1();

    void getControlInfo(unsigned long pos, bool& hole, bool& trackZero, bool& writeProtect);

    void step(bool direction);
    void selectSide(BYTE side);

    BYTE readData(unsigned long pos);
    void writeData(unsigned long pos, BYTE data);

    virtual BYTE readSectorData(BYTE sector, unsigned long pos);
    void insertDisk(FloppyDisk* disk);


  private:

    const unsigned int  maxTracks_c = 40;
    const unsigned char head_c      = 0;
};

#endif // H_17_1_H_
