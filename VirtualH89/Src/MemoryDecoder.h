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

#include "Memory8K.h"
#include "MemoryLayout.h"
#include "GppListener.h"

class MemoryDecoder: public GppListener
{
  public:
    MemoryDecoder(int numBanks, BYTE gppBits): GppListener(gppBits) {
        numBnks = numBanks;
        bnkMask = numBanks - 1; // assumes power of 2
        banks   = new MemoryLayout*[numBanks];
        curBank = 0;
        GppListener::addListener(this);
    }
    virtual ~MemoryDecoder() {
    }
    virtual void reset()                             = 0;
    virtual void addLayout(int ix, MemoryLayout* lo) = 0;
    inline MemoryLayout* getLayout(int ix) {
        return banks[ix & bnkMask];
    }
    inline int numLayouts() {
        return numBnks;
    }
    inline int getCurrentBank() {
        return curBank;
    }

    inline BYTE readByte(int ix, WORD adr) {
        return getLayout(ix)->getPage(adr)->readByte(adr);
    }
    inline void writeByte(int ix, WORD adr, BYTE val)
    {
        getLayout(ix)->getPage(adr)->writeByte(adr, val);
    }
  protected:
    BYTE           curBank;
    int            bnkMask;
    int            numBnks;
    MemoryLayout** banks;
};

#endif // MEMORYDECODER_H_
