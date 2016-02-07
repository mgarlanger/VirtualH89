/// \file h89-io.h
///
/// \date Mar 7, 2009
/// \author Mark Garlanger
///

#ifndef H89_IO_H_
#define H89_IO_H_

#include "config.h"
#include "h89Types.h"

#include "IODevice.h"
#include <vector>

///
/// \brief %H89 IO Bus
///
/// Interfaces between the CPU and all the I/O devices
///
class H89_IO
{
  public:
    H89_IO();
    ~H89_IO();

    std::vector<IODevice *>& getDiskDevices();
    bool addDiskDevice(IODevice *device);
    bool addDevice(IODevice *device);
    bool removeDevice(IODevice *device);

    BYTE in(BYTE addr);
    void out(BYTE addr, BYTE val);

  private:
    IODevice *iodevices[256];
    std::vector<IODevice *> dsk_devs;
};

#endif // H89_IO_H_
