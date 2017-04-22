///
///  \file IMDFloppyDisk.h
///
///  \author Mark Garlanger
///  \date April, 17 2016
///

#ifndef IMDFLOPPYDISK_H_
#define IMDFLOPPYDISK_H_

#include "GenericFloppyDisk.h"

/// \cond
#include <vector>
#include <memory>
/// \endcond

class DiskSide;
class Track;
class Sector;

class IMDFloppyDisk: public GenericFloppyDisk
{
  public:
    IMDFloppyDisk(std::vector<std::string> argv);
    virtual ~IMDFloppyDisk() override;


    virtual bool readData(BYTE track,
                          BYTE side,
                          BYTE sector,
                          int  inSector,
                          int& data) override;
    virtual bool writeData(BYTE track,
                           BYTE side,
                           BYTE sector,
                           int  inSector,
                           BYTE data,
                           bool dataReady,
                           int& result) override;
    virtual bool isReady() override;
    virtual void eject(const std::string name) override;
    virtual void dump(void) override;
    bool findSector(BYTE side,
                    BYTE track,
                    BYTE sector) override;

    static GenericFloppyDisk_ptr getDiskette(std::vector<std::string> argv);

  private:
    const std::string                          imageName_m;
    static const unsigned int                  maxHeads_c = 2;

    std::vector <std::shared_ptr<Track> >      tracks_m[maxHeads_c];
    std::vector<std::shared_ptr<DiskSide> >    sideData_m;

    std::shared_ptr<Sector>                    curSector_m;
    int                                        dataPos_m;
    int                                        sectorLength_m;
    BYTE                                       secLenCode_m;
    bool                                       ready_m;

    // int           gapLen_m;
    // int           indexGapLen_m;
    // unsigned long writePos_m;
    // bool          trackWrite_m;


  protected:
    bool readIMD(const char* name);


};


#endif // IMDFLOPPYDISK_H_
