///
/// \file MemoryDecoder.h
///
/// Base class for memory decoders, which implement various layouts of
/// the address space along with switching between them.
///
/// \date Mar 05, 2016
/// \author Douglas Miller
///
#ifndef MEMORYDECODER_H_
#define MEMORYDECODER_H_

#include "MemoryLayout.h"
#include "GppListener.h"

#include "Memory8K.h"

class MemoryDecoder: public GppListener
{
  public:
    MemoryDecoder(int numBanks, BYTE gppBits);
    virtual ~MemoryDecoder();

    virtual void reset()                             = 0;
    virtual void addLayout(int ix, MemoryLayout* lo) = 0;

    inline MemoryLayout* getLayout(int ix)
    {
        return banks_m[ix & bankMask_m];
    }

    inline int numLayouts()
    {
        return numBanks_m;
    }

    inline int getCurrentBank()
    {
        return curBank_m;
    }

    inline BYTE readByte(int ix, WORD adr)
    {
        return getLayout(ix)->getPageByAddress(adr)->readByte(adr);
    }

    inline void writeByte(int ix, WORD adr, BYTE val)
    {
        getLayout(ix)->getPageByAddress(adr)->writeByte(adr, val);
    }

    inline BYTE readByte(WORD address)
    {
        return getLayout(curBank_m)->getPageByAddress(address)->readByte(address);
    }
    inline void writeByte(WORD address, BYTE val)
    {
        getLayout(curBank_m)->getPageByAddress(address)->writeByte(address, val);
    }


  protected:
    BYTE           curBank_m;
    int            bankMask_m;
    int            numBanks_m;
    MemoryLayout** banks_m;
};

#endif // MEMORYDECODER_H_
