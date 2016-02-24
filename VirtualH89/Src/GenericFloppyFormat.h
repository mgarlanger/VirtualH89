/// \file GenericFloppyFormat.h
///
/// Some constants and conventions used to represent floppy diskette formats.
///
/// \date  Feb 1, 2016
/// \author Douglas Miller
///

#ifndef GENERICFLOPPYFORMAT_H_
#define GENERICFLOPPYFORMAT_H_

#include "config.h"
#include "h89Types.h"

///
/// \brief Virtual Generic Floppy Drive
///
/// Implements a virtual floppy disk drive. Supports 48/96 tpi 5.25",
/// 48 tpi 8", either can be SS or DS. Note, the media determines density.
///
class GenericFloppyFormat
{
  public:
    enum AMbytes
    {
        INDEX_AM_BYTE = 0xfc,
        ID_AM_BYTE    = 0xfe,
        DATA_AM_BYTE  = 0xfb,
        INDEX_AM      = -INDEX_AM_BYTE,
        ID_AM         = -ID_AM_BYTE,
        DATA_AM       = -DATA_AM_BYTE,
        ERROR         = -1,
    };

  private:
};

#endif // GENERICFLOPPYFORMAT_H_
