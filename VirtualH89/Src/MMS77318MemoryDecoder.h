///
/// \file MMS77318MemoryDecoder.h
///
/// Definition of the MMS 77318 128K add-on RAM card and
/// associated decoder modification.
///
/// \date Mar 05, 2016
/// \author Douglas Miller
///
#ifndef MMS77318MEMORYDECODER_H_
#define MMS77318MEMORYDECODER_H_

#include "MemoryDecoder.h"

class MMS77318MemoryDecoder: public MemoryDecoder
{
  public:
    MMS77318MemoryDecoder(shared_ptr<MemoryLayout> h89_0);
    virtual ~MMS77318MemoryDecoder();

    virtual void reset();
  private:
    virtual void gppNewValue(BYTE gpo);
    static const BYTE h89_gppBnkSelBit0_c  = 0b00100000;
    static const BYTE h89_gppBnkSelBit1_c  = 0b00000100;
    static const BYTE h89_gppBnkSelBit2_c  = 0b00010000;
    static const BYTE h89_gppBnkSelBits_c  = (h89_gppBnkSelBit0_c |
                                              h89_gppBnkSelBit1_c |
                                              h89_gppBnkSelBit2_c);
    static const BYTE h89_gppUnlockBits_c  = 0b00001100;
    static const BYTE h89_gppWatchedBits_c = (h89_gppBnkSelBits_c |
                                              h89_gppUnlockBits_c);

    int               lockState;
    static const BYTE lockSeq[];
};

#endif // MMS77318MEMORYDECODER_H_
