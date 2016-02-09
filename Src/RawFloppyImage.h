/// \file RawFloppyImage.h
///
/// \brief virtual Floppy Disk
///
/// \date Feb 2, 2016
///
/// \author Douglas Miller
///

#ifndef RAWFLOPPYIMAGE_H_
#define RAWFLOPPYIMAGE_H_

#include "config.h"
#include "h89Types.h"
#include "GenericFloppyDrive.h"
#include "GenericFloppyFormat.h"
#include <sys/types.h>
#include <string>
#include <vector>

/// \class RawFloppyImage
///
/// \brief A virtual floppy disk
///
class RawFloppyImage: public GenericFloppyDisk
{
  public:
    RawFloppyImage(GenericFloppyDrive *drive, std::vector<std::string> argv);
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
