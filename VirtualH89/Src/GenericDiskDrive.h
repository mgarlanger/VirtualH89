/// \file GenericDiskDrive.h
///
/// Interface for a floppy disk drive, supporting methods for handling
/// mounted media.
///
/// \date  Feb 1, 2016
/// \author Douglas Miller
///

#ifndef GENERICDISKDRIVE_H_
#define GENERICDISKDRIVE_H_

/// \cond
#include <string>
#include <memory>
/// \endcond

class GenericFloppyDisk;

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

    virtual std::string getMediaName()                               = 0;
    virtual void insertDisk(std::shared_ptr<GenericFloppyDisk> disk) = 0;

    // Returns the number of raw bytes per track, lowest density.
    virtual int getRawBytesPerTrack()                                = 0;
    virtual int getNumTracks()                                       = 0;
    virtual bool isReady()                                           = 0;
    virtual bool isWriteProtect()                                    = 0;

  private:
};

#endif // GENERICDISKDRIVE_H_
