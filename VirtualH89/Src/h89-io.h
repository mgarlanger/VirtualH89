/// \file h89-io.h
///
/// \brief Interfaces between the CPU and all the I/O devices
///
/// \date Mar 7, 2009
/// \author Mark Garlanger
///

#ifndef H89_IO_H_
#define H89_IO_H_

#include "IOBus.h"

/// \cond
#include <vector>
/// \endcond



class IODevice;
class DiskController;

///
/// \brief %H89 IO Bus
///
/// Interfaces between the CPU and all the I/O devices
///
///
class H89_IO: public IOBus
{
  public:
    H89_IO();
    virtual ~H89_IO() override;

    std::vector<DiskController*>& getDiskDevices();
    virtual bool addDiskDevice(DiskController* device);

  private:
    std::vector<DiskController*> dsk_devs;

};

#endif // H89_IO_H_
