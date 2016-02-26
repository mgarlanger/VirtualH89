///
/// \file RAMemory8K.h
///
/// An 8K page of RAM (R/W).
///
/// \date Mar 05, 2016
/// \author Douglas Miller
///
#ifndef RAMEMORY8K_H_
#define RAMEMORY8K_H_

#include <string.h>

#include "h89Types.h"
#include "Memory8K.h"

class RAMemory8K: public Memory8K
{
  public:
    RAMemory8K(WORD ba):
        Memory8K(ba) {
    }
    virtual void writeByte(WORD adr, BYTE val) {
        mem[adr & 0x1fff] = val;
    }
  protected:
};

#endif // RAMEMORY8K_H_
