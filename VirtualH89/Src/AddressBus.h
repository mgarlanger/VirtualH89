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

#include "config.h"
#include "h89Types.h"

const int addressLine_c  = 16;
const int memorySize_c   = 1L << addressLine_c;
const int pageSizeBits_c = 10;
const int pageSize_c     = 1L << pageSizeBits_c;
const int numOfPages_c   = memorySize_c / pageSize_c;

class Memory;
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
    /// \todo - should I have a read and a write memory separate to handle the lower
    ///         8k RAM/ROM, so that writes get written out to memory... but how do
    ///         I handle less than 64k of memory... ?? start at 8k and have it wrap around???
    Memory*              mem[numOfPages_c];
    InterruptController* ic_m;

  public:
    AddressBus(InterruptController* ic);
    virtual ~AddressBus();

    // setup memory
    void installMemory(Memory* mem);
    virtual void clearMemory(BYTE data = 0);

    InterruptController* getIntrCtrlr();
    void setIntrCtrlr(InterruptController* ic);
    // Wanted to use the [] operator, but unfortunately it can't be used since the
    // emulated CPU may need to handle writing and reading differently.
    BYTE readByte(WORD addr,
                  bool interruptAck = false);
    void writeByte(WORD addr,
                   BYTE val);

};

#endif // ADDRESSBUS_H_
