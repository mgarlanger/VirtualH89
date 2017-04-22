/// \file SectorFloppyImage.h
///
/// \brief virtual Floppy Disk implementation for soft-sectored media.
/// Supports media image files that have a sector image of the disk.
///
/// \date Feb 2, 2016
///
/// \author Douglas Miller
///

#ifndef SECTORFLOPPYIMAGE_H_
#define SECTORFLOPPYIMAGE_H_


#include "h89Types.h"
#include "GenericFloppyDisk.h"

/// \cond
#include <sys/types.h>
#include <string>
#include <vector>
#include <memory>
/// \endcond

class GenericDiskDrive;

/// \class SectorFloppyImage
///
/// \brief A virtual floppy disk
///
class SectorFloppyImage: public GenericFloppyDisk
{
  public:
    SectorFloppyImage(GenericDiskDrive* drive, std::vector<std::string> argv);
    virtual ~SectorFloppyImage() override;
    static std::shared_ptr<GenericFloppyDisk> getDiskette(GenericDiskDrive*        drive,
                                                          std::vector<std::string> argv);

    bool readData(BYTE track, BYTE side, BYTE sector, int inSector, int& data) override;
    bool writeData(BYTE track, BYTE side, BYTE sector, int inSector,
                   BYTE data, bool dataReady, int& result) override;
    bool findSector(BYTE side,
                    BYTE track,
                    BYTE sector) override;
    bool isReady() override;
    void eject(const std::string name) override;
    void dump(void) override;
    std::string getMediaName() override;

  private:
    const char*   imageName_m;
    int           imageFd_m;
    BYTE*         secBuf_m;
    int           bufferedTrack_m;
    int           bufferedSide_m;
    int           bufferedSector_m;
    off_t         bufferOffset_m;
    bool          bufferDirty_m;
    bool          hypoTrack_m;  // ST media in DT drive
    bool          hyperTrack_m; // DT media in ST drive
    bool          interlaced_m;
    int           mediaLat_m;
    BYTE          secLenCode_m;
    int           gapLen_m;
    int           indexGapLen_m;
    unsigned long writePos_m;
    bool          trackWrite_m;
    int           dataPos_m;
    int           dataLen_m;

    bool checkHeader(BYTE* buf, int n);
    bool cacheSector(int side, int track, int sector);

  protected:

};

#endif // SECTORFLOPPYIMAGE_H_
