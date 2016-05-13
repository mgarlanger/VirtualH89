/// \file wd1797.cpp
///
/// Implementation of the WD1797 FDC (floppy disk controller) chip.
///
/// \date Feb 1, 2016
/// \author Douglas Miller, cloned from h37.cpp by Mark Garlanger
///

#include "wd1797.h"

#include "H89.h"
#include "logger.h"
#include "GenericFloppyDrive.h"
#include "GenericFloppyFormat.h"
#include "WD179xUserIf.h"
#include "WallClock.h"

// 8" (2MHz) step rates, 5.25" (1MHz) are 2x.
const BYTE WD1797::speeds[maxStepSpeeds_c] = {3, 6, 10, 15};

const int  WD1797::sectorLengths[2][4] =
{
    {256, 512, 1024, 128}, // [0]
    {128, 256, 512,  1024} // [1]
};

void
WD1797::updateReady(GenericFloppyDrive* drive)
{

    if (drive->isWriteProtect())
    {
        statusReg_m |= stat_WriteProtect_c;
    }
    else
    {
        statusReg_m &= ~stat_WriteProtect_c;
    }


    bool ready;

    // H37 has ready pin tied to +5V.
    if (userIf_m)
    {
        ready = userIf_m->readReady();
    }
    else
    {
        ready = drive->isReady();
    }

    if (ready)
    {
        statusReg_m &= ~stat_NotReady_c;
    }
    else
    {
        statusReg_m |= stat_NotReady_c;
    }
}


WD1797::WD1797(int baseAddr): ClockUser(),
                              basePort_m(baseAddr),
                              trackReg_m(0),
                              sectorReg_m(0),
                              dataReg_m(0),
                              cmdReg_m(0),
                              statusReg_m(0),
                              dataReady_m(false),
                              intrqRaised_m(false),
                              drqRaised_m(false),
                              headLoaded_m(false),
                              sectorLength_m(0),
                              lastIndexStatus_m(false),
                              indexCount_m(0),
                              stepUpdate_m(false),
                              stepSettle_m(0),
                              missCount_m(0),
                              seekSpeed_m(0),
                              verifyTrack_m(false),
                              multiple_m(false),
                              delay_m(false),
                              side_m(0),
                              deleteDAM_m(false),
                              curCommand_m(noneCmd),
                              stepDirection_m(dir_out),
                              curPos_m(0),
                              sectorPos_m(InitialSectorPos_c),
                              userIf_m(nullptr),
                              cycleCount_m(0)
{

}

WD1797::WD1797(WD179xUserIf* userIf): WD1797(0)
{
    userIf_m = userIf;
}

WD1797::~WD1797()
{

}

void
WD1797::reset(void)
{
    trackReg_m        = 0;
    sectorReg_m       = 0;
    dataReg_m         = 0;
    cmdReg_m          = 0;
    statusReg_m       = 0;
    dataReady_m       = false;
    intrqRaised_m     = false;
    drqRaised_m       = false;
    headLoaded_m      = false;
    sectorLength_m    = 0;
    lastIndexStatus_m = false;
    indexCount_m      = 0;
    stepUpdate_m      = false;
    stepSettle_m      = 0;
    missCount_m       = 0;
    seekSpeed_m       = 0;
    verifyTrack_m     = false;
    multiple_m        = false;
    delay_m           = false;
    side_m            = 0;
    deleteDAM_m       = false;
    curCommand_m      = noneCmd;
    stepDirection_m   = dir_out;
    // leave curPos_m alone, diskette is still spinning...
    sectorPos_m       = InitialSectorPos_c;
}



BYTE
WD1797::in(BYTE addr)
{
    BYTE offset = addr - basePort_m;
    BYTE val    = 0;

    debugss(ssWD1797, ALL, "(%d)\n", addr);

    switch (offset)
    {
        case StatusPort_Offset_c:
            debugss(ssWD1797, VERBOSE, "(StatusPort) (%02x) trk=%02x sec=%02x dat=%02x\n",
                    statusReg_m, trackReg_m, sectorReg_m, dataReg_m);
            val = statusReg_m;
            lowerIntrq();
            break;

        case TrackPort_Offset_c:
            debugss(ssWD1797, VERBOSE, "(TrackPort) - %d\n", trackReg_m);
            val = trackReg_m;
            break;

        case SectorPort_Offset_c:
            debugss(ssWD1797, VERBOSE, "(SectorPort) - %d\n", sectorReg_m);
            val = sectorReg_m;
            break;

        case DataPort_Offset_c:
            // TODO - should this check dataReady_m first?
            debugss(ssWD1797, VERBOSE, "(DataPort) - %d\n", dataReg_m);
            val          = dataReg_m;
            dataReady_m  = false;
            statusReg_m &= ~stat_DataRequest_c;
            lowerDrq();
            break;

        default:
            debugss(ssWD1797, ERROR, "(Unknown - 0x%02x)\n", addr);
            break;

    }

    return (val);
}

void
WD1797::out(BYTE addr,
            BYTE val)
{
    BYTE offset = addr - basePort_m;

    debugss(ssWD1797, ALL, "(%d, %d (0x%x))\n", addr, val, val);

    switch (offset)
    {
        case CommandPort_Offset_c:
            debugss(ssWD1797, VERBOSE, "(CommandPort): %02x trk=%02x sec=%02x dat=%02x\n",
                    val, trackReg_m, sectorReg_m, dataReg_m);
            // MUST tolerate changing of selected disk *after* setting command...
            // timing is tricky...
            indexCount_m = 0;
            cmdReg_m     = val;
            processCmd(val);
            break;

        case TrackPort_Offset_c:
            debugss(ssWD1797, VERBOSE, "(TrackPort): %d\n", val);
            trackReg_m = val;
            break;

        case SectorPort_Offset_c:
            debugss(ssWD1797, VERBOSE, "(SectorPort): %d\n", val);
            sectorReg_m = val;
            break;

        case DataPort_Offset_c:
            debugss(ssWD1797, VERBOSE, "(DataPort): %02x\n", val);

            // unpredictable results if !dataReady_m... (data changed while being written).
            // other mechanisms detect lostData, which is different.
            dataReg_m   = val;
            dataReady_m = true;
            lowerDrq();
            break;

        default:
            debugss(ssWD1797, ERROR, "(Unknown - 0x%02x): %d\n", addr, val);
            break;
    }
}

void
WD1797::waitForData()
{
    h89.waitCPU();
}

void
WD1797::processCmd(BYTE cmd)
{
    debugss(ssWD1797, INFO, "cmd: 0x%02x\n", cmd);

    // First check for the Force Interrupt command
    if ((cmd & cmd_Mask_c) == cmd_ForceInterrupt_c)
    {
        processCmdTypeIV(cmd);
        return;
    }

    // Make sure controller is not already busy. Documentation
    // did not specify what would happen, it mentions 'This register should not
    // be loaded when the device is busy unless the new command is a force interrupt.'
    // for now, start the new command.
    if ((statusReg_m & stat_Busy_c) == stat_Busy_c)
    {
        debugss(ssWD1797, WARNING, "New command while still busy: %d\n", cmd);
        // TODO determine the proper behavior.
        // return;
    }

    // Can't reference drive here... it could be about to change.
    // GenericFloppyDrive *drive = getCurDrive();

    // set Busy flag - clear everything else
    statusReg_m = stat_Busy_c;
    debugss(ssWD1797, INFO, "Setting busy flag\n")

    if ((cmd & 0x80) == 0x00)
    {
        // Type I commands
        processCmdTypeI(cmd);
    }
    else if ((cmd & 0x40) == 0x00)
    {
        // Type II commands
        processCmdTypeII(cmd);
    }
    else
    {
        // must be Type III command
        processCmdTypeIII(cmd);
    }
}

void
WD1797::processCmdTypeI(BYTE cmd)
{
    verifyTrack_m = ((cmd & cmdop_VerifyTrack_c) != 0);
    seekSpeed_m   = speeds[cmd & cmdop_StepMask_c];
    lowerDrq();
    dataReady_m   = false;

    if (getClockPeriod() > 500)
    {
        seekSpeed_m *= 2;
    }

    loadHead((cmd & cmdop_HeadLoad_c) != 0);
    stepUpdate_m = false;

    debugss(ssWD1797, INFO, "cmd: %d\n", cmd);
    // reset CRC Error, Seek Error, DRQ, INTRQ
    statusReg_m &= ~(stat_CRCError_c | stat_SeekError_c);
    lowerDrq();
    lowerIntrq();

    if ((cmd & 0xf0) == 0x00)
    {
        debugss(ssWD1797, INFO, "Restore\n");
        curCommand_m = restoreCmd;
    }
    else if ((cmd & 0xc0) == 0x00)
    {
        debugss(ssWD1797, INFO, "Seek\n");
        curCommand_m = seekCmd;
        // For the 1797 version, there are two bit patterns that
        // are defined as SEEK,  0 0 0 1 x x x x and 0 0 1 T x x x x
        // wasn't clear in the documentation if the first one updated the track
        // reg, but assuming it does.
        stepUpdate_m = (cmd & cmdop_TrackUpdate_c) != 0;
    }
    else
    {
        // One of the Step commands
        debugss(ssWD1797, INFO, "Step\n");
        curCommand_m = stepCmd;

        stepUpdate_m = (cmd & cmdop_TrackUpdate_c) != 0;

        // check for step in or step out.
        if ((cmd & 0x40) == 0x40)
        {
            if ((cmd & 0x20) == 0x20)
            {
                // Step Out
                debugss(ssWD1797, INFO, "Step Out\n");
                stepDirection_m = dir_out;
            }
            else
            {
                // Step In
                debugss(ssWD1797, INFO, "Step In\n");
                stepDirection_m = dir_in;
            }
        }
    }

    // Drive selection might change, need to delay start of command...
    stepSettle_m = 50;

    // TODO: detect track0
    // statusReg_m |= stat_TrackZero_c;

}

void
WD1797::processCmdTypeII(BYTE cmd)
{
    multiple_m     = ((cmd & cmdop_MultipleRecord_c) != 0);
    delay_m        = ((cmd & cmdop_Delay_15ms_c) != 0);
    sectorLength_m = ((cmd & cmdop_SectorLength_c) != 0 ? 1 : 0);
    side_m         = ((cmd & cmdop_UpdateSSO_c) >> cmdop_UpdateSSO_Shift_c);
    loadHead(true);


    debugss(ssWD1797, INFO, "cmd: %d\n", cmd);
    lowerDrq();
    dataReady_m = false;
    sectorPos_m = InitialSectorPos_c;

    if ((cmd & 0x20) == 0x20)
    {
        // Write Sector
        deleteDAM_m = ((cmd & cmdop_DataAddressMark_c) != 0);

        if (deleteDAM_m)
        {
            debugss(ssWD1797, ERROR, "Deleted Data Addr Mark not supported - ignored\n");
        }

        debugss(ssWD1797, INFO, "Write Sector - \n");
        curCommand_m = writeSectorCmd;
    }
    else
    {
        // Read Sector
        debugss(ssWD1797, INFO,
                "Read Sector: %d - multi: %d delay: %d sectorLength_m: %d side: %d\n",
                sectorReg_m, multiple_m, delay_m, sectorLength_m, side_m);
        curCommand_m = readSectorCmd;
    }

    stepSettle_m = HeadSettleTimeInTicks_c; // give host time to get ready...
}

void
WD1797::processCmdTypeIII(BYTE cmd)
{
    delay_m     = ((cmd & cmdop_Delay_15ms_c) != 0);
    side_m      = ((cmd & cmdop_UpdateSSO_c) >> cmdop_UpdateSSO_Shift_c);
    loadHead(true);
    lowerDrq();
    dataReady_m = false;
    sectorPos_m = InitialSectorPos_c;

    debugss(ssWD1797, INFO, "cmd: %d\n", cmd);

    if ((cmd & 0xf0) == 0xc0)
    {
        // Read Address
        debugss(ssWD1797, INFO, "Read Address\n");
        curCommand_m = readAddressCmd;

    }
    else if ((cmd & 0xf0) == 0xf0)
    {
        // write Track
        debugss(ssWD1797, INFO, "Write Track: %d\n", trackReg_m);
        curCommand_m = writeTrackCmd;
        raiseDrq();

    }
    else if ((cmd & 0xf0) == 0xe0)
    {
        // read Track
        debugss(ssWD1797, INFO, "Read Track: %d\n", trackReg_m);
        curCommand_m = readTrackCmd;

    }
    else
    {
        debugss(ssWD1797, ERROR, "Invalid type-III cmd: %x\n", cmd);
        statusReg_m &= ~stat_Busy_c;
        return;
    }

    stepSettle_m = HeadSettleTimeInTicks_c; // give host time to get ready...
}

// 'drive' might be NULL.
void
WD1797::processCmdTypeIV(BYTE cmd)
{
    debugss(ssWD1797, INFO, "cmd: 0x%02x\n", cmd);
    loadHead(false);
    // we assume drive won't change for Type IV commands...
    GenericFloppyDrive* drive = getCurDrive();

    curCommand_m = forceInterruptCmd;

    // check to see if previous command is still running
    if (statusReg_m & stat_Busy_c)
    {
        debugss(ssWD1797, INFO, "Aborting Command\n");
        // still running, abort command and reset busy
        abortCmd();
        statusReg_m &= ~stat_Busy_c;
    }
    else if (drive)
    {
        // no Command running, update status.
        // try more-surgical repair of status register.
        statusReg_m &= ~stat_SeekError_c;
        statusReg_m &= ~stat_CRCError_c;
        // if previous command was Tyype II or III, need to clear
        // stat_LostData_c and stat_DataRequest_c ? But not if Type I.
        debugss(ssWD1797, INFO, "updating statusReg: %d\n", statusReg_m);
    }
    else
    {
        debugss(ssWD1797, ERROR, "Type IV with no drive\n");
        statusReg_m |= stat_NotReady_c;
    }

    if ((cmd & 0x0f) != 0x00)
    {
        // TODO: Fix this, must setup background "task" waiting for event...

        // at least one bit is set.
        if ((cmd & cmdop_NotReadyToReady_c) == cmdop_NotReadyToReady_c)
        {
            debugss(ssWD1797, ERROR, "Not Ready to Ready - not implemented\n");

        }

        if ((cmd & cmdop_ReadyToNotReady_c) == cmdop_ReadyToNotReady_c)
        {
            debugss(ssWD1797, ERROR, "Ready to Not Ready - not implemented\n");

        }

        if ((cmd & cmdop_IndexPulse_c) == cmdop_IndexPulse_c)
        {
            debugss(ssWD1797, ERROR, "Index Pulse - not implemented\n");

        }

        if ((cmd & cmdop_ImmediateInterrupt_c) == cmdop_ImmediateInterrupt_c)
        {
            debugss(ssWD1797, INFO, "Immediate Interrupt\n");
            statusReg_m &= ~stat_Busy_c;
            raiseIntrq();
        }
    }
    else
    {
        debugss(ssWD1797, INFO, "No Interrupt/ Clear Busy\n");
        statusReg_m &= ~stat_Busy_c;
        curCommand_m = noneCmd;
    }
}

void
WD1797::abortCmd()
{
    debugss(ssWD1797, INFO, "\n");

    curCommand_m = noneCmd;
}

void
WD1797::raiseIntrq()
{
    debugss(ssWD1797, INFO, "\n");
    intrqRaised_m = true;
    if (userIf_m)
    {
        userIf_m->raiseIntrq();
    }
}

void
WD1797::raiseDrq()
{
    debugss(ssWD1797, INFO, "\n");
    drqRaised_m = true;
    if (userIf_m)
    {
        userIf_m->raiseDrq();
    }

}

void
WD1797::lowerIntrq()
{
    debugss(ssWD1797, INFO, "\n");
    intrqRaised_m = false;
    if (userIf_m)
    {
        userIf_m->lowerIntrq();
    }

}

void
WD1797::lowerDrq()
{
    debugss(ssWD1797, INFO, "\n");
    drqRaised_m = false;
    if (userIf_m)
    {
        userIf_m->lowerDrq();
    }

}

bool
WD1797::doubleDensity()
{
    if (!userIf_m)
    {
        return false;
    }

    return userIf_m->doubleDensity();

}

void
WD1797::loadHead(bool load)
{
    debugss(ssWD1797, INFO, "%sload\n", load ? "" : "un");
    headLoaded_m = load;
}

GenericFloppyDrive*
WD1797::getCurDrive()
{
    if (!userIf_m)
    {
        return nullptr;
    }

    return userIf_m->getCurrentDrive();
}


int
WD1797::getClockPeriod()
{
    if (!userIf_m)
    {
        return 1000;
    }

    return userIf_m->getClockPeriod();
}

void
WD1797::transferData(int data)
{
    if (dataReady_m)
    {
        statusReg_m |= stat_LostData_c;
    }

    dataReady_m  = true;
    dataReg_m    = data;
    statusReg_m |= stat_DataRequest_c;
    raiseDrq();
}

bool
WD1797::checkAddr(BYTE addr[6])
{
    return (addr[0] == trackReg_m && addr[1] == side_m && addr[2] == sectorReg_m);
}

int
WD1797::sectorLen(BYTE addr[6])
{
    int x = addr[3] & 0x03;
    return sectorLengths[sectorLength_m][x];
}

void
WD1797::notification(unsigned int cycleCount)
{

    cycleCount_m += cycleCount;

    unsigned long       charPos   = 0;
    GenericFloppyDrive* drive     = getCurDrive();
    bool                indexEdge = false;

    if (!drive)
    {

        // TODO: set some status indicator.
        bool notReady = true;
        // the H37 has the ready signal tied to +5.
        if (userIf_m && userIf_m->readReady())
        {
            notReady = false;
        }

        if (notReady)
        {
            statusReg_m |= stat_NotReady_c;

            // TODO only type II & III commands are aborted on a not ready
            if (curCommand_m != noneCmd)
            {
                abortCmd();
                // TODO: shouldn't abortCmd() do all this?
                raiseIntrq();
                statusReg_m &= ~stat_Busy_c;
            }

            return;
        }
        // TODO - Type one commands are not affected by the ready input, those
        // should still continue, but currently no protection for drive-> uses
        // below.
        return;
    }

    statusReg_m &= ~stat_NotReady_c;

    drive->notification(cycleCount);

    if (drive->getIndexPulse())
    {
        if (!lastIndexStatus_m)
        {
            indexEdge = true;
            ++indexCount_m;
        }

        lastIndexStatus_m = true;
    }
    else
    {
        lastIndexStatus_m = false;
    }

    updateReady(drive);

    if (stepSettle_m > 0)
    {
        if (stepSettle_m > cycleCount)
        {
            stepSettle_m -= cycleCount;
            return;
        }

        else
        {
            stepSettle_m = 0;
            missCount_m  = 0;
        }
    }

    charPos = drive->getCharPos(doubleDensity());

    if (charPos == curPos_m)
    {
        // TODO density shouldn't affect timing for later - and shouldn't depend on drive/disk
        // Position hasn't changed just return
        return;
    }

    debugss(ssWD1797, ALL, "New character Pos - old: %ld, new: %ld\n", curPos_m, charPos);
    curPos_m = charPos;

    switch (curCommand_m)
    {
        case restoreCmd:
        {
            // TODO - set track to 255, and error out if it gets to zero before drive reports zero
            debugss(ssWD1797, ALL, "restoreCmd\n");
            if (!drive->getTrackZero())
            {
                drive->step(false);
                stepSettle_m = HeadSettleTimeInTicks_c; // millisecToTicks(seekSpeed_m);
            }
            else
            {
                trackReg_m   = 0;
                statusReg_m |= stat_TrackZero_c;
                statusReg_m &= ~stat_Busy_c;
                raiseIntrq();
                curCommand_m = noneCmd;
            }

            break;
        }

        case seekCmd:
            if (dataReg_m != trackReg_m)
            {
                bool dir = (dataReg_m > trackReg_m);
                drive->step(dir);
                trackReg_m  += (dir ? 1 : -1);
                stepSettle_m = HeadSettleTimeInTicks_c; // millisecToTicks(seekSpeed_m);
            }
            else
            {
                if (verifyTrack_m)
                {
                    int track, sector, side;

                    // TODO: confirm this works...
                    if (!drive->readAddress(track, sector, side))
                    {
                        statusReg_m |= stat_CRCError_c;
                    }
                    else if (track != trackReg_m)
                    {
                        statusReg_m |= stat_SeekError_c;
                    }
                }

                if (drive->getTrackZero())
                {
                    statusReg_m |= stat_TrackZero_c;
                }
                else
                {
                    statusReg_m &= ~stat_TrackZero_c;
                }

                statusReg_m &= ~stat_Busy_c;
                raiseIntrq();
                curCommand_m = noneCmd;
            }

            break;

        case stepCmd:
            if (stepDirection_m == dir_out)
            {
                debugss(ssWD1797, INFO, "step out\n");

                if (!drive->getTrackZero())
                {
                    drive->step(false);

                    if (drive->getTrackZero())
                    {
                        statusReg_m |= stat_TrackZero_c;
                    }
                    else
                    {
                        statusReg_m &= ~stat_TrackZero_c;
                    }

                    stepSettle_m = HeadSettleTimeInTicks_c; // millisecToTicks(seekSpeed_m);

                    if (stepUpdate_m)
                    {
                        trackReg_m--;
                    }
                }
                else
                {
                    statusReg_m |= stat_TrackZero_c;
                }
            }
            else if (stepDirection_m == dir_in)
            {
                debugss(ssWD1797, INFO, "step in\n");

                drive->step(true);
                statusReg_m &= ~stat_TrackZero_c;
                stepSettle_m = HeadSettleTimeInTicks_c; // millisecToTicks(seekSpeed_m);

                if (stepUpdate_m)
                {
                    trackReg_m++;
                }
            }

            curCommand_m = stepDoneCmd;
            break;

        case stepDoneCmd:
            if (drive->getTrackZero())
            {
                statusReg_m |= stat_TrackZero_c;
            }
            else
            {
                statusReg_m &= ~stat_TrackZero_c;
            }

            statusReg_m &= ~stat_Busy_c;
            raiseIntrq();
            curCommand_m = noneCmd;
            break;

        case noneCmd:
            break;

        default:
            debugss(ssWD1797, WARNING, "default1: %d\n", curCommand_m);
            break;
    }

    int data;
    int result;

    switch (curCommand_m)
    {
        case restoreCmd:
        case seekCmd:
        case stepCmd:
        case stepDoneCmd: // won't match, but added to silence compiler warning
        case noneCmd:
            updateReady(drive);

            if (lastIndexStatus_m)
            {
                statusReg_m |= stat_IndexPulse_c;
            }
            else
            {
                statusReg_m &= ~stat_IndexPulse_c;
            }

            break;

        case readSectorCmd:

            debugss(ssWD1797, WARNING, "readSectorCmd - dataReady_m: %d\n", dataReady_m);
            // user may choose to ignore data... must not hang here!
            if (dataReady_m)
            {
                debugss(ssWD1797, WARNING, "readSectorCmd\n");
                if ((statusReg_m & stat_LostData_c) == 0 && ++missCount_m < 4)
                {
                    debugss(ssWD1797, WARNING, "missCount_m - %d\n", missCount_m);

                    // wait a little for host to catch up
                    break;
                }
                debugss(ssWD1797, ERROR, "Lost Data\n");
                statusReg_m |= stat_LostData_c;
            }

            missCount_m = 0;
            drive->selectSide(side_m);
            debugss(ssWD1797, WARNING, "track: %d, sector: %d, sectorPos_m- %d\n", trackReg_m,
                    sectorReg_m, sectorPos_m);
            data = drive->readData(doubleDensity(), trackReg_m,
                                   side_m, sectorReg_m, sectorPos_m);
            debugss(ssWD1797, WARNING, "data- %d\n", data);

            if (data == GenericFloppyFormat::NO_DATA)
            {
                debugss(ssWD1797, WARNING, "no data\n");
                // just wait for sector to come around..
            }
            else if (data == GenericFloppyFormat::DATA_AM)
            {
                debugss(ssWD1797, WARNING, "DATA_AM\n");
                sectorPos_m = 0;
            }
            else if (data == GenericFloppyFormat::CRC)
            {
                BYTE sectors = drive->getMaxSectors(side_m, trackReg_m);
                debugss(ssWD1797, WARNING, "CRC - sectors: %d\n", sectors);
                /// \TODO determine proper way to know the number of sectors on the disk.

                if (!multiple_m || sectorReg_m == sectors) // drive->getMaxSectors(side_m, trackReg_m))
                {
                    sectorPos_m  = ErrorSectorPos_c;
                    statusReg_m &= ~stat_Busy_c;
                    raiseIntrq();
                    curCommand_m = noneCmd;
                }
                else
                {
                    sectorReg_m++;
                    sectorPos_m = InitialSectorPos_c;
                }
            }
            else if (data < 0)
            {
                debugss(ssWD1797, WARNING, "less than zero\n");
                // probably ERROR
                sectorPos_m  = ErrorSectorPos_c;
                statusReg_m |= stat_CRCError_c;
                statusReg_m &= ~stat_Busy_c;
                raiseIntrq();
                curCommand_m = noneCmd;
            }
            else
            {
                transferData(data);
                ++sectorPos_m;
            }

            break;

        case readAddressCmd:

            // user may choose to ignore data... must not hang here!
            if (dataReady_m)
            {
                if ((statusReg_m & stat_LostData_c) == 0 && ++missCount_m < 4)
                {
                    // wait a little for host to catch up
                    break;
                }

                statusReg_m |= stat_LostData_c;
            }

            missCount_m = 0;
            drive->selectSide(side_m);
            // sector '0xfd' indicates a read address
            data        = drive->readData(doubleDensity(), trackReg_m, side_m,
                                          0xfd, sectorPos_m);

            if (data == GenericFloppyFormat::NO_DATA)
            {
                // just wait for sector to come around..
                // should never happen, as long as track was formatted.
            }
            else if (data == GenericFloppyFormat::ID_AM)
            {
                sectorPos_m = 0;
            }
            else if (data == GenericFloppyFormat::CRC)
            {
                debugss(ssWD1797, INFO, "read address %d - done\n", sectorPos_m);
                sectorPos_m  = ErrorSectorPos_c;
                statusReg_m &= ~stat_Busy_c;
                raiseIntrq();
                curCommand_m = noneCmd;
            }
            else if (data < 0)
            {
                // probably ERROR
                sectorPos_m  = ErrorSectorPos_c;
                statusReg_m |= stat_CRCError_c;
                statusReg_m &= ~stat_Busy_c;
                raiseIntrq();
                curCommand_m = noneCmd;
            }
            else
            {
                if (sectorPos_m == 0)
                {
                    sectorReg_m = data;
                }

                debugss(ssWD1797, INFO, "read address %d - %02x\n", sectorPos_m, data);

                transferData(data);
                ++sectorPos_m;
            }

            break;

        case writeSectorCmd:
            drive->selectSide(side_m);
            result = drive->writeData(doubleDensity(), trackReg_m, side_m, sectorReg_m,
                                      sectorPos_m, dataReg_m, dataReady_m);

            if (result == GenericFloppyFormat::NO_DATA)
            {
                // out of paranoia, but must be careful
                if (sectorPos_m >= 0 && !drqRaised_m)
                {
                    raiseDrq();
                }

                // just wait for sector to come around..
            }
            else if (result == GenericFloppyFormat::DATA_AM)
            {
                sectorPos_m = 0;
            }
            else if (result == GenericFloppyFormat::CRC)
            {
                sectorPos_m  = ErrorSectorPos_c;
                statusReg_m &= ~stat_Busy_c;
                raiseIntrq();
                curCommand_m = noneCmd;
            }
            else if (result < 0)
            {
                // other errors
                sectorPos_m  = ErrorSectorPos_c;
                statusReg_m |= stat_WriteFault_c;
                statusReg_m &= ~stat_Busy_c;
                raiseIntrq();
                curCommand_m = noneCmd;
            }
            else
            {
                dataReady_m = false;
                ++sectorPos_m;
                raiseDrq();
            }

            break;

        case readTrackCmd:

            // user may choose to ignore data... must not hang here!
            if (dataReady_m)
            {
                if ((statusReg_m & stat_LostData_c) == 0 && ++missCount_m < 4)
                {
                    // wait a little for host to catch up
                    break;
                }

                statusReg_m |= stat_LostData_c;
            }

            missCount_m = 0;
            drive->selectSide(side_m);
            data        = drive->readData(doubleDensity(), trackReg_m, side_m,
                                          0xff, sectorPos_m);

            if (data == GenericFloppyFormat::NO_DATA)
            {
                // just wait for index to come around..
            }
            else if (data == GenericFloppyFormat::INDEX_AM)
            {
                sectorPos_m = 0;
            }
            else if (data == GenericFloppyFormat::CRC)
            {
                sectorPos_m  = ErrorSectorPos_c;
                statusReg_m &= ~stat_Busy_c;
                raiseIntrq();
                curCommand_m = noneCmd;
            }

            else if (data < 0)
            {
                // probably ERROR
                sectorPos_m  = ErrorSectorPos_c;
                statusReg_m |= stat_CRCError_c;
                statusReg_m &= ~stat_Busy_c;
                raiseIntrq();
                curCommand_m = noneCmd;
            }

            else
            {
                transferData(data);
                ++sectorPos_m;
            }

            break;

        case writeTrackCmd:

            drive->selectSide(side_m);

            // \todo
            result = drive->writeData(doubleDensity(), trackReg_m, side_m, 0xff,
                                      sectorPos_m, dataReg_m, dataReady_m);

            if (result == GenericFloppyFormat::NO_DATA)
            {
                // out of paranoia, but must be careful
                if (sectorPos_m >= 0 && !drqRaised_m)
                {
                    raiseDrq();
                }

                // just wait for sector to come around..
            }
            else if (result == GenericFloppyFormat::INDEX_AM)
            {
                sectorPos_m = 0;
            }
            else if (result == GenericFloppyFormat::CRC)
            {
                sectorPos_m  = ErrorSectorPos_c;
                statusReg_m &= ~stat_Busy_c;
                raiseIntrq();
                curCommand_m = noneCmd;
            }
            else if (result < 0)
            {
                // other errors
                debugss(ssWD1797, ERROR, "Error in write track\n");
                sectorPos_m  = ErrorSectorPos_c;
                statusReg_m |= stat_WriteFault_c;
                statusReg_m &= ~stat_Busy_c;
                raiseIntrq();
                curCommand_m = noneCmd;
            }
            else
            {
                dataReady_m = false;
                ++sectorPos_m;
                raiseDrq();
            }

            break;

        case forceInterruptCmd:
            // TODO: watch for event(s) and raise interrupt when seen...
            break;
    }

}

unsigned long
WD1797::millisecToTicks(unsigned long ms)
{
    unsigned long tps   = WallClock::instance()->getTicksPerSecond();
    unsigned long ticks = (tps * ms) / 1000;
    return ticks;
}
