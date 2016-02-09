/// \file GenericFloppyDrive.h
///
/// \date  Feb 1, 2016
/// \author Douglas Miller
///

#ifndef GENERICDISKDRIVE_H_
#define GENERICDISKDRIVE_H_

#include "config.h"
#include "h89Types.h"
#include "GenericDiskDrive.h"
#include <string>

///
/// \brief Virtual Generic Disk Drive
///
/// Implements a virtual floppy disk drive. Supports 48/96 tpi 5.25",
/// 48 tpi 8", either can be SS or DS. Note, the media determines density.
///
class GenericDiskDrive
{
  public:

    GenericDiskDrive();

    virtual ~GenericDiskDrive();

    virtual std::string getMediaName() = 0;

  private:
};

#endif // GENERICDISKDRIVE_H_
