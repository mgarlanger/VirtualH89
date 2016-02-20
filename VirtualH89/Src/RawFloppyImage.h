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

#include <sys/types.h>
#include <string>
#include <vector>

#include "config.h"
#include "h89Types.h"
#include "GenericFloppyDrive.h"
#include "GenericFloppyFormat.h"

/// \class RawFloppyImage
///
/// \brief A virtual floppy disk
///
class RawFloppyImage: public GenericFloppyDisk
{
  public:
    RawFloppyImage(GenericDiskDrive *drive, std::vector<std::string> argv);
    ~RawFloppyImage();

    bool readData(BYTE side, BYTE track, unsigned int pos, int& data);
    bool startWrite(BYTE side, BYTE track, unsigned int pos);
    bool stopWrite(BYTE side, BYTE track, unsigned int pos);
    bool writeData(BYTE side, BYTE track, unsigned int pos, BYTE data);
    bool isReady();
    void eject(const char *name);
    void dump(void);
    std::string getMediaName();

  private:
    const char *imageName_m;
    int imageFd_m;
    BYTE *trackBuffer_m;
    int bufferedTrack_m;
    int bufferedSide_m;
    off_t bufferOffset_m;
    bool bufferDirty_m;
    bool hypoTrack_m; // ST media in DT drive
    bool hyperTrack_m; // DT media in ST drive
    bool interlaced_m;
    int gapLen_m;
    int indexGapLen_m;
    unsigned long writePos_m;
    bool trackWrite_m;

    void getAddrMark(BYTE *tp, int nbytes, int& id_tk, int& id_sd, int& id_sc, int& id_sl);
    bool cacheTrack(int side, int track);

  protected:

};

#endif // RAWFLOPPYIMAGE_H_
