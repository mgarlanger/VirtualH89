///
/// \file H89MemoryDecoder.h
///
/// H89 56K+ROM or 64K layout
///
/// \date Mar 05, 2016
/// \author Douglas Miller
///
#ifndef H89MEMORYDECODER_H_
#define H89MEMORYDECODER_H_

#include "MemoryDecoder.h"


class H89MemoryDecoder: public MemoryDecoder
{
  public:
    H89MemoryDecoder(shared_ptr<MemoryLayout> h89_0);
    virtual ~H89MemoryDecoder();

  private:
    virtual void gppNewValue(BYTE gpo);
    static const BYTE h89_gppOrg0Bit_c = 0b00100000;
};

#endif // H89MEMORYDECODER_H_
