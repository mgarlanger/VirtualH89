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


class Memory8K
{
  public:
    Memory8K(WORD base);

    inline WORD getBase() {
        return base_m;
    }
    inline BYTE readByte(WORD adr) {
        return mem[adr & 0x1fff];
    }
    virtual void writeByte(WORD adr, BYTE val) = 0;

  protected:
    WORD base_m;
    BYTE mem[8 * 1024];
};

#endif // MEMORY8K_H_
