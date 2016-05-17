///
/// \file H88MemoryDecoder.h
///
/// Basic H88 48K RAM+ROM layout
///
/// \date Mar 05, 2016
/// \author Douglas Miller
///
#ifndef H88MEMORYDECODER_H_
#define H88MEMORYDECODER_H_

#include "MemoryDecoder.h"


class H88MemoryDecoder: public MemoryDecoder
{
  public:
    H88MemoryDecoder(shared_ptr<MemoryLayout> h89_0);
    virtual ~H88MemoryDecoder();

  private:
    virtual void gppNewValue(BYTE gpo);

    static const BYTE h89_gppOrg0Bit_c = 0b00100000;
};

#endif // H88MEMORYDECODER_H_
