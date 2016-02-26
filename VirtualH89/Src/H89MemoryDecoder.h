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
#include "MemoryLayout.h"

class H89MemoryDecoder: public MemoryDecoder
{
  public:
    H89MemoryDecoder(MemoryLayout* h89_0);
    virtual ~H89MemoryDecoder();

    virtual void addLayout(int ix, MemoryLayout* lo);
    virtual void reset();
  private:
    virtual void gppNewValue(BYTE gpo);
    static const BYTE h89_gppOrg0Bit_c = 0b00100000;
};

#endif // H89MEMORYDECODER_H_
