///
/// \name crc-ccitt.h
///
///
/// \date Oct 5, 2012
/// \author Mark Garlanger
///

#ifndef CRC_CCITT_H_
#define CRC_CCITT_H_

#include "h89Types.h"

class CRC_CCITT
{
  public:

    CRC_CCITT();
    ~CRC_CCITT();

    void initCRC();
    void updateCRC(BYTE val);
    WORD getCRC();

  private:
    WORD              crc_m;

    static const WORD initValue_c = 0xffff;
    static const WORD crctable[256];

};



#endif // CRC_CCITT_H_
