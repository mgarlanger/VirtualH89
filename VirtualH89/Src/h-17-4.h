///
/// \file h-17-4.h
///
/// \date Jul 21, 2012
/// \author Mark Garlanger
///

#ifndef H_17_4_H_
#define H_17_4_H_

#include "DiskDrive.h"

class FloppyDisk;

/// \brief Virtual double-sided 96 tpi floppy disk drive.
///
/// Implements a virtual floppy disk drive. The H-17-4 is a double-sided, 80 track drive.
/// On a H-17 controller, this yields 160 total track and 1600 total sectors. Each sector
/// is 256 bytes, this provides support for about 400k per drive.
/// Upcoming support for the Z-89-37 controller will allow this drive to store up to 720K.
///
class H_17_4: public DiskDrive
{
  public:
    H_17_4();
    virtual ~H_17_4();

    void getControlInfo(unsigned long pos,
                        bool&         hole,
                        bool&         trackZero,
                        bool&         writeProtect);

    void selectSide(BYTE side);
    void step(bool direction);

    BYTE readData(unsigned long pos);
    void writeData(unsigned long pos,
                   BYTE          data);

    virtual BYTE readSectorData(BYTE          sector,
                                unsigned long pos);
    void insertDisk(FloppyDisk* disk);

  private:

    BYTE                      side_m;
    BYTE                      track_m;

    static const unsigned int maxTracks_c = 80;
};


#endif // H_17_4_H_
