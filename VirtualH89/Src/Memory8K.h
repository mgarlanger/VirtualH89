///
/// \file Memory8K.h
///
/// An 8K page of RAM.
///
/// \date Mar 05, 2016
/// \author Douglas Miller
///
#ifndef MEMORY8K_H_
#define MEMORY8K_H_

#include "h89Types.h"

#include <string.h>

class Memory8K
{
  public:
    Memory8K(WORD ba) {
        base = ba;
        memset(mem, 0, sizeof(mem));
    }
    inline WORD getBase() {
        return base;
    }
    inline BYTE readByte(WORD adr) {
        return mem[adr & 0x1fff];
    }
    virtual void writeByte(WORD adr, BYTE val) = 0;
  protected:
    WORD base;
    BYTE mem[8 * 1024];
};

#endif // MEMORY8K_H_
