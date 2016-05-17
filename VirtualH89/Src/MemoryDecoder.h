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
#include <vector>

class MemoryDecoder: public GppListener
{
  public:
    MemoryDecoder(int numBanks, BYTE gppBits);
    virtual ~MemoryDecoder();

    virtual void reset();

    virtual void addLayout(int                      ix,
                           shared_ptr<MemoryLayout> lo);

    inline shared_ptr<MemoryLayout> getLayout(int ix)
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

    inline BYTE readByte(int  ix,
                         WORD adr)
    {
        return getLayout(ix)->getPageByAddress(adr)->readByte(adr);
    }

    inline void writeByte(int ix, WORD adr,
                          BYTE val)
    {
        getLayout(ix)->getPageByAddress(adr)->writeByte(adr, val);
    }

    inline BYTE readByte(WORD address)
    {
        return curLayout_m->getPageByAddress(address)->readByte(address);
    }
    inline void writeByte(WORD address,
                          BYTE val)
    {
        curLayout_m->getPageByAddress(address)->writeByte(address, val);
    }


  protected:
    virtual void updateCurBank(BYTE bank);
    BYTE                                   curBank_m;
    int                                    bankMask_m;
    int                                    numBanks_m;

    std::vector<shared_ptr<MemoryLayout> > banks_m;

    shared_ptr<MemoryLayout>               curLayout_m;
};

#endif // MEMORYDECODER_H_
