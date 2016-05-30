///
/// \file H88MemoryDecoder.h
///
/// Basic H88 48K RAM+ROM layout, this memory decoder has no support
/// for ORG-0 functionality
///
/// \date Mar 05, 2016
/// \author Douglas Miller
///
#ifndef H88MEMORYDECODER_H_
#define H88MEMORYDECODER_H_

#include "MemoryDecoder.h"

class SystemMemory8K;

class H88MemoryDecoder: public MemoryDecoder
{
  public:
    H88MemoryDecoder(std::shared_ptr<SystemMemory8K>  systemRam,
                     MemoryLayout::MemorySize_t       memSize = MemoryLayout::Mem_48k);

    virtual ~H88MemoryDecoder();

  private:
    const static unsigned NumMemoryLayouts_c = 1;
    const static BYTE     gppMask_c          = 0;
    void                  gppNewValue(BYTE);
};

#endif // H88MEMORYDECODER_H_
