///
/// \name EightInchDisk.h
///
///
/// \date Aug 5, 2013
/// \author Mark Garlanger
///

#ifndef EIGHTINCHDISK_H_
#define EIGHTINCHDISK_H_

#include "SoftSectoredDisk.h"

class EightInchDisk: public SoftSectoredDisk
{
public:
    EightInchDisk();
    EightInchDisk(const char *name, SoftSectoredDisk::DiskImageFormat format);

    virtual ~EightInchDisk();

//    virtual bool readData(BYTE side, BYTE track, unsigned int pos, BYTE &data);
//    virtual bool writeData(BYTE side, BYTE track, unsigned int pos, BYTE data);
//    virtual void getControlInfo(unsigned int pos, bool &hole, bool &writeProtect);
//    virtual void setWriteProtect(bool value);
//    virtual bool checkWriteProtect(void);

//    virtual bool readSectorData(BYTE side, BYTE track, BYTE sector, unsigned int pos, BYTE &data);

//    virtual void eject(const char *name);

//    virtual void dump(void);

private:


};

#endif // EIGHTINCHDISK_H_
