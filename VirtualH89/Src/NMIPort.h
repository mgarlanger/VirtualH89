///
/// \name NMIPort.h
///
/// \brief Non-Maskable Interrupt port
///
/// \date Jul 15, 2012
/// \author Mark Garlanger
///

#ifndef NMIPORT_H_
#define NMIPORT_H_

#include "IODevice.h"

///
/// \brief Non-Maskable-Interrupt  (NMI) Port
///
/// On the H89, a few of the ports defined on the H8 are not implemented. In order to
/// allow software to function on both system, when these addresses are accessed a NMI
/// is generated.
///
class NMIPort: public virtual IODevice
{
  public:
    NMIPort(BYTE base, BYTE size);
    virtual ~NMIPort();

    virtual BYTE in(BYTE addr);
    virtual void out(BYTE addr, BYTE val);
    void reset() {
    }
  private:

};

#endif /// NMIPORT_H_
