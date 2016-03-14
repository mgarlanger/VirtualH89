///
/// \file NilMemory8K.h
///
/// Implementation of an 8K page of non-existent memory. Reads 0, writes ignored.
///
/// \date Mar 05, 2016
/// \author Douglas Miller
///
#ifndef NILMEMORY8K_H_
#define NILMEMORY8K_H_

#include "Memory8K.h"

class NilMemory8K: public Memory8K
{
  public:
    NilMemory8K(WORD base):
        Memory8K(base)
    {
    }

    virtual ~NilMemory8K() {
    }

    void writeByte(WORD adr, BYTE val) {
    }
};

#endif // NILMEMORY8K_H_
