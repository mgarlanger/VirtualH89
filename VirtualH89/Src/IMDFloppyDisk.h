///
///  \file IMDFloppyDisk.h
///
///  \author Mark Garlanger
///  \date April, 17 2016
///

#ifndef IMDFLOPPYDISK_H_
#define IMDFLOPPYDISK_H_

#include "GenericFloppyDisk.h"

#include <vector>

class Track;
class Sector;

class IMDFloppyDisk: public GenericFloppyDisk
{
  public:
    IMDFloppyDisk(std::vector<std::string> argv);
    virtual ~IMDFloppyDisk();


    virtual bool readData(BYTE track,
                          BYTE side,
                          BYTE sector,
                          int  inSector,
                          int& data);
    virtual bool writeData(BYTE track,
                           BYTE side,
                           BYTE sector,
                           int  inSector,
                           BYTE data,
                           bool dataReady,
                           int& result);
    virtual BYTE getMaxSectors(BYTE sides,
                               BYTE track);
    virtual bool isReady();
    virtual void eject(const char* name);
    virtual void dump(void);
    virtual std::string getMediaName();
    static GenericFloppyDisk* getDiskette(std::vector<std::string> argv);

  private:
    const char*               imageName_m;
    static const unsigned int maxHeads_c = 2;

    std::vector <Track*>      tracks_m[maxHeads_c];

    Sector*                   curSector_m;
    int                       dataPos_m;
    int                       sectorLength_m;
    BYTE                      secLenCode_m;
    bool                      ready_m;

    bool                      hypoTrack_m;  // ST media in DT drive
    bool                      hyperTrack_m; // DT media in ST drive
    // bool          interlaced_m;
    // int           mediaLat_m;
    // BYTE          secLenCode_m;
    // int           gapLen_m;
    // int           indexGapLen_m;
    // unsigned long writePos_m;
    // bool          trackWrite_m;
    // int           dataPos_m;
    // int           dataLen_m;

    // bool checkHeader(BYTE* buf, int n);
    // bool cacheSector(int side, int track, int sector);

  protected:
    bool readIMD(const char* name);
    bool findSector(int side, int track, int sector);

};


#endif // IMDFLOPPYDISK_H_
