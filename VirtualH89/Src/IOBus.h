///
///  \file IOBus.h
///
///  \author Mark Garlanger
///  \date April 16, 2016.
///
///  Copyright © 2016 Mark Garlanger. All rights reserved.
///

#ifndef IOBUS_H
#define IOBUS_H

#include "h89Types.h"
#include <vector>


class IODevice;
class DiskController;

class IOBus
{
  public:
    IOBus();
    virtual ~IOBus();

    virtual bool addDevice(IODevice* device);
    virtual bool removeDevice(IODevice* device);
    virtual void reset();

    virtual BYTE in(BYTE addr);
    virtual void out(BYTE addr,
                     BYTE val);

  protected:
    IODevice*   iodevices[256];


};



#endif // IOBUS_H
