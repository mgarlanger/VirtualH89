/// \file GenericFloppyDrive.cpp
///
/// Implementation of a generic floppy disk drive, should be capable of
/// supporting any type of floppy drive, 8", 5.25", DS, DT, etc.
///
/// \date Feb 2, 2016
/// \author Douglas Miller
///

#include "GenericFloppyDrive.h"

#include "WallClock.h"
#include "logger.h"
#include "GenericFloppyFormat.h"
#include "GenericFloppyDisk.h"


using namespace std;

GenericFloppyDrive::GenericFloppyDrive(unsigned int heads,
                                       unsigned int tracks,
                                       unsigned int mediaSize): numHeads_m(heads),
                                                                numTracks_m(tracks),
                                                                mediaSize_m(mediaSize),
                                                                track_m(0),
                                                                headSel_m(0),
                                                                cycleCount_m(0),
                                                                disk_m(nullptr),
                                                                writeProtected_m(false)
{
    // Can this change on-the-fly?
    ticksPerSec_m = WallClock::instance()->getTicksPerSecond();

    if (mediaSize_m == 8)
    {
        driveRpm_m           = 360;
        rawSDBytesPerTrack_m = 6400;
    }
    else if (mediaSize_m == 5)
    {
        driveRpm_m           = 300;
        rawSDBytesPerTrack_m = 3200;
    }

    ticksPerRev_m = (ticksPerSec_m * 60) / driveRpm_m;
    motor_m       = (mediaSize_m == 8);
    headLoaded_m  = (mediaSize_m == 5);
}

GenericFloppyDrive*
GenericFloppyDrive::getInstance(std::string type)
{
    unsigned int heads;
    unsigned int tracks;
    unsigned int mediaSize;

    if (type.find("FDD_5_25") == 0)
    {
        mediaSize = 5;

        if (type.find("ST") != std::string::npos)
        {
            tracks = 40;
        }
        else if (type.find("DT") != std::string::npos)
        {
            tracks = 80;
        }
        else
        {
            debugss(ssGenericFloppyDrive, ERROR, "number of tracks not specified\n");
            return nullptr;
        }
    }
    else if (type.find("FDD_8") == 0)
    {
        mediaSize = 8;
        tracks    = 77;
    }
    else
    {
        debugss(ssGenericFloppyDrive, ERROR, "disk size not specified\n");
        return NULL;
    }

    if (type.find("SS") != std::string::npos)
    {
        heads = 1;
    }
    else if (type.find("DS") != std::string::npos)
    {
        heads = 2;
    }
    else
    {
        debugss(ssGenericFloppyDrive, ERROR, "number of sides not specified\n");
        return nullptr;
    }

    return new GenericFloppyDrive(heads, tracks, mediaSize);
}

GenericFloppyDrive::~GenericFloppyDrive()
{

}

void
GenericFloppyDrive::insertDisk(shared_ptr<GenericFloppyDisk> disk)
{

    disk_m = disk;
    if (disk_m)
    {
        disk_m->setDriveType(numTracks_m);
        writeProtected_m = disk_m->checkWriteProtect();

    }
    else
    {
        writeProtected_m = false;
    }
}

bool
GenericFloppyDrive::getTrackZero()
{
    return (track_m == 0);
}

void
GenericFloppyDrive::step(bool direction)
{
    if (direction)
    {
        if (track_m < numTracks_m - 1)
        {
            ++track_m;
        }

        debugss(ssGenericFloppyDrive, INFO, "in(up) (%d)\n", track_m);
    }
    else
    {
        if (track_m > 0)
        {
            --track_m;
        }

        debugss(ssGenericFloppyDrive, INFO, "out(down) (%d)\n", track_m);
    }
}

void
GenericFloppyDrive::selectSide(BYTE side)
{
    debugss(ssGenericFloppyDrive, VERBOSE, "side: %d\n");
    headSel_m = side % numHeads_m;
}

// negative data is "missing clock" detection.
int
GenericFloppyDrive::readData(bool dd,
                             BYTE track,
                             BYTE side,
                             BYTE sector,
                             int  inSector)
{
    int data = 0;
    debugss(ssGenericFloppyDrive, INFO, "side:%d,track:%d,sector:%d,sector pos %d\n", side, track,
            sector, inSector);

    if (!disk_m)
    {
        debugss(ssGenericFloppyDrive, WARNING, "no Disk\n");
        return GenericFloppyFormat::ERROR;
    }

    if (dd != disk_m->doubleDensity())
    {
        debugss(ssGenericFloppyDrive, WARNING, "DD mismatch(%d)\n", dd);
        return GenericFloppyFormat::ERROR;
    }

    if (track_m != track || headSel_m != side)
    {
        debugss(ssGenericFloppyDrive, WARNING, "mismatch trk %d:%d sid %d:%d\n", track_m, track,
                headSel_m, side);
    }

    // override FDC track/side with our own - it's the real one
    if (disk_m->readData(track_m, headSel_m, sector, inSector, data))
    {
        debugss(ssGenericFloppyDrive, INFO,
                "read passed - dd: %d, track: %d, sctor: %d, pos(%d) data(%d)\n", dd, track, sector,
                inSector, data);
    }
    else
    {
        debugss(ssGenericFloppyDrive, WARNING, "readData failed\n");
    }

    return data;
}

int
GenericFloppyDrive::writeData(bool dd,
                              BYTE track,
                              BYTE side,
                              BYTE sector,
                              int  inSector,
                              BYTE data,
                              bool dataReady)
{
    int result = GenericFloppyFormat::NO_DATA;

    if (!disk_m)
    {
        return GenericFloppyFormat::ERROR;
    }

    if (sector == 0xff)
    {
        if (!dd)
        {
            sector &= ~1;
        }
    }

    else if (dd != disk_m->doubleDensity())
    {
        return GenericFloppyFormat::ERROR;
    }

    if ((track_m != track) || (headSel_m != side))
    {
        debugss(ssGenericFloppyDrive, ERROR, "track/head mismatch - track(%d - %d) head(%d - %d)\n",
                track_m, track, headSel_m, side);
    }
    // override FDC track/side with our own - it's the real one
    if (!disk_m->writeData(track_m, headSel_m, sector, inSector, data, dataReady, result))
    {
        debugss(ssGenericFloppyDrive, INFO, "write failed - pos(%d) data(%d)\n",
                inSector, data);
    }

    return result;
}


void
GenericFloppyDrive::notification(unsigned int cycleCount)
{
    if (disk_m == NULL || !motor_m)
    {
        return;
    }

    cycleCount_m += cycleCount;
    cycleCount_m %= ticksPerRev_m;
    // TODO: what is appropriate width of index pulse?
    // TODO - this should be pushed down to the floppy disk
    // indexPulse_m  = (cycleCount_m < 100); // approx 50uS...
    indexPulse_m = (cycleCount_m < 2000); // approx 50uS...
}

unsigned long
GenericFloppyDrive::getCharPos(bool doubleDensity)
{
    // if disk_m == NULL || !motor_m then cycleCount_m won't be updating
    // and so CharPos also does not update.  Callers checks this.
    unsigned long bytes = rawSDBytesPerTrack_m;

    if (doubleDensity)
    {
        bytes *= 2;
    }

    unsigned long ticksPerByte = ticksPerRev_m / bytes;
    return (cycleCount_m / ticksPerByte);
}

bool
GenericFloppyDrive::readAddress(int& trackNum,
                                int& sectorNum,
                                int& sideNum)
{
    if (disk_m == NULL || !motor_m)
    {
        debugss(ssGenericFloppyDrive, ERROR, "drive or motor not set %d:%d\n", disk_m == NULL,
                motor_m);
        return false;
    }

    // consult media to see if it knows.
    trackNum  = disk_m->getRealTrackNumber(track_m);
    sectorNum = 0; // TODO: use charPos and media to approximate
    sideNum   = headSel_m;
    return true;
}


bool
GenericFloppyDrive::verifyTrackSector(BYTE trackNum,
                                      BYTE sectorNum)
{
    if (disk_m == NULL || !motor_m)
    {
        debugss(ssGenericFloppyDrive, ERROR, "drive or motor not set %d:%d\n", disk_m == NULL,
                motor_m);
        return false;
    }

    BYTE realTrackNum = disk_m->getRealTrackNumber(track_m);

    if (realTrackNum != trackNum)
    {
        debugss(ssGenericFloppyDrive, ERROR, "realTrack/track: %d:%d\n", realTrackNum, trackNum);
        return false;
    }

    return disk_m->findSector(headSel_m, realTrackNum, sectorNum);
}

void
GenericFloppyDrive::headLoad(bool load)
{
    if (mediaSize_m == 8)
    {
        headLoaded_m = load;
    }
}

void
GenericFloppyDrive::motor(bool on)
{
    if (mediaSize_m == 5)
    {
        motor_m = on;
    }
}

bool
GenericFloppyDrive::isReady()
{
    return (disk_m != NULL && disk_m->isReady());
}

bool
GenericFloppyDrive::isWriteProtect()
{
    return (disk_m != NULL && disk_m->checkWriteProtect());
}

std::string
GenericFloppyDrive::getMediaName()
{
    return (disk_m != NULL ? disk_m->getMediaName() : "");
}

void
GenericFloppyDrive::startTrackFormat(BYTE trackNum)
{

}

void
GenericFloppyDrive::getDriveStatus(bool& writeProtected,
                                   bool& headLoaded,
                                   bool& trackZero,
                                   bool& indexPulse)
{
    writeProtected = writeProtected_m;
    headLoaded     = headLoaded_m;
    trackZero      = (track_m == 0);
    indexPulse     = indexPulse_m;
}
