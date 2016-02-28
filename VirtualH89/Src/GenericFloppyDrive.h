/// \file GenericFloppyDrive.h
///
/// Implementation of a generic floppy disk drive, should be capable of
/// supporting any type of floppy drive, 8", 5.25", DS, DT, etc.
///
/// \date  Feb 1, 2016
/// \author Douglas Miller
///

#ifndef GENERICFLOPPYDRIVE_H_
#define GENERICFLOPPYDRIVE_H_

#include "GenericDiskDrive.h"

class GenericFloppyDisk;

///
/// \brief Virtual Generic Floppy Drive
///
/// Implements a virtual floppy disk drive. Supports 48/96 tpi 5.25",
/// 48 tpi 8", either can be SS or DS. Note, the media determines density.
///
class GenericFloppyDrive: public GenericDiskDrive
{
  public:
    enum DriveType
    {
        FDD_5_25_SS_ST = 1,
        FDD_5_25_SS_DT,
        FDD_5_25_DS_ST,
        FDD_5_25_DS_DT,
        FDD_8_SS,
        FDD_8_DS,
    };

    GenericFloppyDrive(DriveType type);

    virtual ~GenericFloppyDrive();

    static GenericFloppyDrive* getInstance(std::string type);
    bool getTrackZero();
    bool readAddress(int& track,
                     int& sector,
                     int& side);

    void step(bool direction);
    void selectSide(BYTE side);

    int readData(bool          doubleDensity,
                 unsigned long pos);
    bool startWrite(bool          doubleDensity,
                    unsigned long pos);
    bool stopWrite(bool          doubleDensity,
                   unsigned long pos);
    bool writeData(bool          doubleDensity,
                   unsigned long pos,
                   BYTE          data);

    void insertDisk(GenericFloppyDisk* disk);

    void notification(unsigned int cycleCount);
    unsigned long getCharPos(bool doubleDensity);

    void headLoad(bool load); // Ignored on 5.25" drives?
    void motor(bool on);      // Ignored on 8" drives
    bool getIndexPulse()
    {
        return indexPulse_m;
    }
    int getNumTracks()
    {
        return numTracks_m;
    }
    int getRawBytesPerTrack()
    {
        return rawSDBytesPerTrack_m;
    }
    bool isReady();
    bool isWriteProtect();

    std::string getMediaName();

  private:
    unsigned int       numTracks_m;
    unsigned int       numHeads_m;
    unsigned int       driveRpm_m;
    unsigned int       mediaSize_m;
    unsigned int       rawSDBytesPerTrack_m;
    unsigned long      ticksPerSec_m;
    unsigned long      ticksPerRev_m;
    unsigned long long cycleCount_m;
    bool               indexPulse_m;

    GenericFloppyDisk* disk_m;
    int                headSel_m;
    int                track_m;
    bool               motor_m;
    bool               head_m;
};

#endif // GENERICFLOPPYDRIVE_H_
