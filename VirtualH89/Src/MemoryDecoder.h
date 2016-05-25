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

#include "GppListener.h"

#include "MemoryLayout.h"
#include "Memory8K.h"

#include <vector>
#include <memory>

class SystemMemory8K;

class MemoryDecoder;

typedef std::shared_ptr<MemoryDecoder> MemoryDecoder_ptr;

class MemoryDecoder: public GppListener
{
  public:

    static MemoryDecoder_ptr createMemoryDecoder(std::string                     type,
                                                 std::shared_ptr<SystemMemory8K> sysMem,
                                                 MemoryLayout::MemorySize_t      memSize);

    MemoryDecoder(int  numLayouts,
                  BYTE gppBits);

    virtual ~MemoryDecoder();

    virtual void reset();

    virtual void addLayout(int              ix,
                           MemoryLayout_ptr lo);

    inline MemoryLayout_ptr getLayout(int ix)
    {
        return layouts_m[ix & layoutMask_m];
    }

    inline int numLayouts();

    inline int getCurrentLayoutNum()
    {
        return curLayoutNum_m;
    }

    inline BYTE readByte(int  ix,
                         WORD addr)
    {
        return getLayout(ix)->getPageByAddress(addr)->readByte(addr);
    }

    inline void writeByte(int  ix,
                          WORD addr,
                          BYTE val)
    {
        getLayout(ix)->getPageByAddress(addr)->writeByte(addr, val);
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

    BYTE                          curLayoutNum_m;
    int                           layoutMask_m;
    int                           numLayouts_m;

    std::vector<MemoryLayout_ptr> layouts_m;

    MemoryLayout_ptr              curLayout_m;

    virtual void updateCurLayout(BYTE layout);
};

#endif // MEMORYDECODER_H_
