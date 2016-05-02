/// \file SerialPortDevice.h
///
/// \date Apr 23, 2009
/// \author Mark Garlanger
///

#ifndef SERIALPORTDEVICE_H_
#define SERIALPORTDEVICE_H_

#include "h89Types.h"

class INS8250;

/// \class SerialPortDevice
///
/// \brief Base class for devices that connect to a Serial Port
///
///
class SerialPortDevice
{
  public:
    SerialPortDevice();
    virtual ~SerialPortDevice();

    virtual void receiveData(BYTE data) = 0;
    virtual bool sendReady();
    virtual bool sendData(BYTE data);

    virtual void attachPort(INS8250* port);

    virtual unsigned int getBaudRate() = 0;
    static const int DISABLE_BAUD_CHECK = -1;

  private:
    INS8250*         port_m;

};

#endif // SERIALPORTDEVICE_H_
