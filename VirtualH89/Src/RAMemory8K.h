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


#include "Memory8K.h"

class RAMemory8K: public Memory8K
{
  public:
    RAMemory8K(WORD baseAddr): Memory8K(baseAddr)
    {
    }
    virtual ~RAMemory8K()
    {
    };

    virtual void writeByte(WORD addr, BYTE val) override
    {
        mem[addr & MemoryAddressMask_c] = val;
    }
  protected:
};

#endif // RAMEMORY8K_H_
