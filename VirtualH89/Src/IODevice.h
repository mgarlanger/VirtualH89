/// \file IODevice.h
///
/// \date Apr 20, 2009
/// \author Mark Garlanger
///

#ifndef IODEVICE_H_
#define IODEVICE_H_

#include "h89Types.h"
#include "config.h"

/// \todo - determine if interrupt level for the device should be here, or if we subclass
///         this to a IOIntrDevice.
///        I'm thinking subclass it with InterruptDevice.

///
/// \brief  Abstract I/O device .
///
/// Base class for all I/O devices.
///
class IODevice
{
  public:
    ///
    ///   \param base Base address for the I/O device
    ///   \param numPorts The number of addresses used by the device.
    ///
    IODevice(BYTE base,
             BYTE numPorts);
    virtual ~IODevice();

    ///
    /// Read from specified port
    ///
    /// \param[in] addr Address of port to read
    ///
    /// \retval The value read from the device at the specified address.
    virtual BYTE in(BYTE addr) = 0;

    ///
    /// Write to specified port
    ///
    /// \param[in] addr Address of port to write
    /// \param[in] val  Value to write to the board
    ///
    virtual void out(BYTE addr, BYTE val) = 0;

    ///
    /// Gets configured base address
    ///
    /// \return The configured base address for this device
    ///
    virtual BYTE getBaseAddress(void);

    ///
    /// Gets number of ports
    ///
    /// \return The number of ports this device uses.
    ///
    virtual BYTE getNumPorts(void);

    ///
    /// Verify of the specified address maps to this device
    ///
    /// \param[in] addr The address to verify
    /// \return If specified address maps to this device
    /// \retval true - Address maps to this device
    /// \retval false - Address does no map to this device.
    ///
    virtual bool verifyPort(BYTE addr);

    ///
    /// Determine the offset of a given port based on
    /// the base address.
    ///
    /// \param[in] addr The address used to determine the offset
    /// \retval offset of the port.
    ///
    virtual BYTE getPortOffset(BYTE addr);

    // System RESET, may be ignored by device - if appropiate
    virtual void reset() = 0;

  protected:

    ///
    /// Base Address for the device
    ///
    BYTE baseAddress_m;

    ///
    /// Number of ports used by this device
    ///
    BYTE numPorts_m;

  private:
    /// Hide default constructor.
    IODevice();

};

#endif // IODEVICE_H_
