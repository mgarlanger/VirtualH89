/// \file ROM.h
///
/// \date Mar 7, 2009
/// \author Mark Garlanger
///

#ifndef ROM_H_
#define ROM_H_

#include "h89Types.h"

/// \brief  Virtual %ROM
///
/// Implementation of Read Only Memory (ROM).
///
class ROM
{
  private:
    /// \brief Actual storage for the rom.
    ///
    BYTE* data_m;
    WORD  base_m;
    WORD  size_m;

  public:
    ROM(int size);
    virtual ~ROM();
    static ROM* getROM(const char* file,
                       WORD        addr);

    virtual void setBaseAddress(WORD adr);
    virtual void initialize(BYTE* block,
                            WORD  size);
    virtual void writeByte(WORD addr,
                           BYTE val);
    virtual BYTE readByte(WORD addr);
    virtual WORD getBase();
    virtual WORD getSize();
    virtual BYTE* getImage();
};

#endif // ROM_H_
