/// \file AddressBus.h
///
///  Abstracts the CPU's Address Bus to allow special handling of
///  memory addresses.
///
///  \date Mar 7, 2009
///  \author Mark Garlanger
///

#ifndef ADDRESSBUS_H_
#define ADDRESSBUS_H_

#include "h89Types.h"


/// \todo make AddressBus a base class and this functionality in an H89AddressBus
/// class

class MemoryDecoder;
class InterruptController;

/// \class AddressBus
///
/// \brief %Memory address bus
///
/// The AddressBus connects the CPU to the computer's memory.
///
class AddressBus
{
  private:
    MemoryDecoder*       mem_m;
    InterruptController* ic_m;

  public:
    AddressBus(InterruptController* ic);
    virtual ~AddressBus();

    // setup memory
    void installMemory(MemoryDecoder* mem);

    // Wanted to use the [] operator, but unfortunately it can't be used since the
    // emulated CPU may need to handle writing and reading differently.
    BYTE readByte(WORD addr,
                  bool interruptAck = false);
    void writeByte(WORD addr,
                   BYTE val);
    void reset();

};

#endif // ADDRESSBUS_H_
