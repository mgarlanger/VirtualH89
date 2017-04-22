/// \file RawFloppyImage.h
///
/// \brief virtual Floppy Disk implementation for soft-sectored media.
/// Supports media image files that have a simplified raw-track format.
/// See GenericFloppyFormat for special format marks INDEX_AM, ID_AM, and
/// DATA_AM which are used to mark the start of respective sections of the media.
/// Gaps are typically zeroes, as opposed to other patterns used in real media.
/// NOTE: On real soft-sectored media the special marks are recorded using a
/// missing-clock method and so the real controller can easily distinguish
/// the marks from actualy data, which could otherwise match the bit pattern.
/// This implemetnation cannot do that in the image file, so it must be able to
/// determine precisely where data and address blocks beging, which means
/// each track must be identically formatted. Real images taken from real media
/// will not conform to that restriction.
///
/// \date Feb 2, 2016
///
/// \author Douglas Miller
///

#ifndef RAWFLOPPYIMAGE_H_
#define RAWFLOPPYIMAGE_H_


#include "GenericFloppyDisk.h"

/// \cond
#include <sys/types.h>
#include <vector>
/// \endcond


class GenericFloppyDrive;
class GenericFloppyFormat;
class GenericDiskDrive;

/// \class RawFloppyImage
///
/// \brief A virtual floppy disk
///
class RawFloppyImage: public GenericFloppyDisk
{
  public:
    RawFloppyImage(GenericDiskDrive*        drive,
                   std::vector<std::string> argv);
    ~RawFloppyImage() override;

    bool readData(BYTE track,
                  BYTE side,
                  BYTE sector,
                  int  inSector,
                  int& data) override;
    bool writeData(BYTE track,
                   BYTE side,
                   BYTE sector,
                   int  inSector,
                   BYTE data,
                   bool dataReady,
                   int& result) override;
    bool isReady() override;
    bool findSector(BYTE side,
                    BYTE track,
                    BYTE sector) override;
    void eject(const std::string name) override;
    void dump(void) override;
    std::string getMediaName() override;

  private:
    const char*   imageName_m;
    int           imageFd_m;
    BYTE*         trackBuffer_m;
    int           bufferedTrack_m;
    int           bufferedSide_m;
    off_t         bufferOffset_m;
    bool          bufferDirty_m;
    bool          hypoTrack_m;  // ST media in DT drive
    bool          hyperTrack_m; // DT media in ST drive
    bool          interlaced_m;
    long          gapLen_m;
    long          indexGapLen_m;
    unsigned long writePos_m;
    bool          trackWrite_m;
    int           headPos_m;
    int           dataPos_m;
    int           dataLen_m;

    void getAddrMark(BYTE* tp,
                     int   nbytes,
                     int&  id_tk,
                     int&  id_sd,
                     int&  id_sc,
                     int&  id_sl);
    bool cacheTrack(int side,
                    int track);
    bool findMark(int mark);
    bool locateSector(BYTE track,
                      BYTE side,
                      BYTE sector);

  protected:

};

#endif // RAWFLOPPYIMAGE_H_
