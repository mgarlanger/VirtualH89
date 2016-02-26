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
#include "MemoryLayout.h"

class H88MemoryDecoder: public MemoryDecoder
{
  public:
    H88MemoryDecoder(MemoryLayout* h89_0);
    virtual ~H88MemoryDecoder();

    virtual void addLayout(int ix, MemoryLayout* lo);
    virtual void reset();
  private:
    virtual void gppNewValue(BYTE gpo);
};

#endif // H88MEMORYDECODER_H_
