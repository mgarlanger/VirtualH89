/// \file RAM.h
///
/// \brief Virtual RAM class.
///
/// \date Mar 7, 2009
/// \author Mark Garlanger
///

#ifndef RAM_H_
#define RAM_H_

#include "Memory.h"

/// \brief Virtual %RAM
///
/// Virtual ram for the %H89 emulator
///
class RAM: public virtual Memory
{
  private:
    BYTE* data_m;

    bool  writeProtect_m;

  public:
    RAM(int size);
    virtual ~RAM();

    virtual void writeByte(WORD addr, BYTE val);
    virtual BYTE readByte(WORD addr);

    virtual void writeProtect(bool wp);
};

#endif // RAM_H_
