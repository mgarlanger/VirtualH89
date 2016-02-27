/// \file FloppyDisk.h
///
/// \brief Base virtual Floppy Disk
///
/// \date May 19, 2009
///
/// \author Mark Garlanger
///

#ifndef FLOPPYDISK_H_
#define FLOPPYDISK_H_

#include "config.h"
#include "h89Types.h"


class DiskData;

/// \class FloppyDisk
///
/// \brief A virtual floppy disk base class.
///
class FloppyDisk
{
  public:
    FloppyDisk();
    FloppyDisk(const char* name);
    virtual ~FloppyDisk();

    virtual bool readData(BYTE          side,
                          BYTE          track,
                          unsigned long pos,
                          BYTE&         data) = 0;
    virtual bool writeData(BYTE          side,
                           BYTE          track,
                           unsigned long pos,
                           BYTE          data)               = 0;
    virtual void getControlInfo(unsigned long pos,
                                bool&         hole,
                                bool&         writeProtect)  = 0;
    virtual void setWriteProtect(bool value);
    virtual bool checkWriteProtect(void);

    virtual bool readSectorData(BYTE  side,
                                BYTE  track,
                                BYTE  sector,
                                WORD  pos,
                                BYTE& data) = 0;

    virtual void eject(const char* name)    = 0;

    virtual void dump(void)                 = 0;

    virtual void setMaxTrack(BYTE maxTrack);
    virtual void setMaxPosition(unsigned int maxPosition);

  private:
    bool         writeProtect_m;
  protected:
    BYTE         maxTrack_m;
    unsigned int maxPos_m;

};


#endif // FLOPPYDISK_H_
