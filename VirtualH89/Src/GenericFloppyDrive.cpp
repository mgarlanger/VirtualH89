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
#include "FloppyDisk.h"
#include "GenericFloppyFormat.h"
#include "GenericFloppyDisk.h"

GenericFloppyDrive::GenericFloppyDrive(DriveType type):
	disk_m(NULL)
{
    // Can this change on-the-fly?
    ticksPerSec_m = WallClock::instance()->getTicksPerSecond();

    if (type == FDD_5_25_DS_ST || type == FDD_5_25_DS_DT || type == FDD_8_DS)
    {
        numHeads_m = 2;
    }
    else
    {
        numHeads_m = 1;
    }

    if (type == FDD_8_SS || type == FDD_8_DS)
    {
        numTracks_m          = 77;
        mediaSize_m          = 8;
        driveRpm_m           = 360;
        rawSDBytesPerTrack_m = 6400;
    }
    else
    {
        mediaSize_m          = 5;
        driveRpm_m           = 300;
        rawSDBytesPerTrack_m = 3200;

        if (type == FDD_5_25_SS_DT || type == FDD_5_25_DS_DT || type == FDD_8_DS)
        {
            numTracks_m = 80;
        }
        else
        {
            numTracks_m = 40;
        }
    }

    cycleCount_m  = 0;
    ticksPerRev_m = (ticksPerSec_m * 60) / driveRpm_m;
    headSel_m     = 0;
    track_m       = 0;
    motor_m       = (mediaSize_m == 8);
    head_m        = (mediaSize_m == 5);
}

GenericFloppyDrive*
GenericFloppyDrive::getInstance(std::string type)
{
    DriveType etype;

    if (type.compare("FDD_5_25_SS_ST") == 0)
    {
        etype = FDD_5_25_SS_ST;
    }
    else if (type.compare("FDD_5_25_SS_DT") == 0)
    {
        etype = FDD_5_25_SS_DT;
    }
    else if (type.compare("FDD_5_25_DS_ST") == 0)
    {
        etype = FDD_5_25_DS_ST;
    }
    else if (type.compare("FDD_5_25_DS_DT") == 0)
    {
        etype = FDD_5_25_DS_DT;
    }
    else if (type.compare("FDD_8_SS") == 0)
    {
        etype = FDD_8_SS;
    }
    else if (type.compare("FDD_8_DS") == 0)
    {
        etype = FDD_8_DS;
    }
    else
    {
        return NULL;
    }

    return new GenericFloppyDrive(etype);
}

GenericFloppyDrive::~GenericFloppyDrive()
{
}


void
GenericFloppyDrive::insertDisk(GenericFloppyDisk* disk)
{
    if (disk_m != NULL)
    {
        delete disk_m;
    }

    disk_m = disk;
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

        debugss(ssGenericFloppyDrive, INFO, "%s - in(up) (%d)\n", __FUNCTION__, track_m);
    }
    else
    {
        if (track_m > 0)
        {
            --track_m;
        }

        debugss(ssGenericFloppyDrive, INFO, "%s - out(down) (%d)\n", __FUNCTION__, track_m);
    }
}

void
GenericFloppyDrive::selectSide(BYTE side)
{
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

    if (!disk_m)
    {
        return GenericFloppyFormat::ERROR;
    }

    if (dd != disk_m->doubleDensity())
    {
        return GenericFloppyFormat::ERROR;
    }

    if (track_m != track || headSel_m != side)
    {
        debugss(ssGenericFloppyDrive, INFO, "mismatch trk %d:%d sid %d:%d\n", track_m, track,
                headSel_m, side);
    }

    // override FDC track/side with our own - it's the real one
    if (disk_m->readData(track_m, headSel_m, sector, inSector, data))
    {
        debugss(ssGenericFloppyDrive, INFO, "%s: read passed - pos(%d) data(%d)\n", __FUNCTION__,
                inSector, data);
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
    int result = GenericFloppyFormat::ERROR;

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

    // override FDC track/side with our own - it's the real one
    if (!disk_m->writeData(track_m, headSel_m, sector, inSector, data, dataReady, result))
    {
        debugss(ssGenericFloppyDrive, INFO, "%s: write failed - pos(%d) data(%d)\n", __FUNCTION__,
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
    indexPulse_m  = (cycleCount_m < 100); // approx 50uS...
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
GenericFloppyDrive::readAddress(int& track,
                                int& sector,
                                int& side)
{
    if (disk_m == NULL || !motor_m)
    {
        return false;
    }

    // For now, just report what we think is there.
    // TODO: consult media to see if it knows.
    track  = track_m;
    sector = 0; // TODO: use charPos and media to approximate
    side   = headSel_m;
    return true;
}

void
GenericFloppyDrive::headLoad(bool load)
{
    if (mediaSize_m == 8)
    {
        head_m = load;
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
    return (disk_m == NULL || disk_m->checkWriteProtect());
}

std::string
GenericFloppyDrive::getMediaName()
{
    return (disk_m != NULL ? disk_m->getMediaName() : "");
}
