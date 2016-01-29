/// \file ROM.h
///
/// \date Mar 7, 2009
/// \author Mark Garlanger
///

#ifndef ROM_H_
#define ROM_H_

#include "config.h"
#include "Memory.h"

/// \brief  Virtual %ROM
///
/// Implementation of Read Only Memory (ROM).
///
class ROM: public virtual Memory
{
  private:
    /// \brief Actual storage for the rom.
    ///
    BYTE *data_m;

  public:
    ROM(int size);
    virtual ~ROM();
    static ROM *getROM(char *file, WORD addr);

    virtual void initialize(BYTE *block, WORD size);
    virtual void writeByte(WORD addr, BYTE val);
    virtual BYTE readByte(WORD addr);
};

#endif // ROM_H_
