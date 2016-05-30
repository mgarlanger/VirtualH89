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

/// \cond
#include <memory>
/// \endcond


class Memory8K
{
  public:
    Memory8K(WORD base);

    WORD getBase();

    inline BYTE readByte(WORD adr)
    {
        return mem[adr & MemoryAddressMask_c];
    }
    virtual void writeByte(WORD adr, BYTE val) = 0;

  protected:
    WORD              base_m;
    BYTE              mem[8 * 1024];
    static const WORD MemoryAddressMask_c = 0x1fff;
};

typedef std::shared_ptr<Memory8K> Memory8K_ptr;

#endif // MEMORY8K_H_
