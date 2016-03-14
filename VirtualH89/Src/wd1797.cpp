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

    if (drive->isReady())
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
                              state_m(idleState),
                              curCommand_m(noneCmd),
                              stepDirection_m(dir_out),
                              curPos_m(0),
                              sectorPos_m(-11)
{
    WallClock::instance()->registerUser(this);
}

WD1797::~WD1797()
{
    WallClock::instance()->unregisterUser(this);
}

void
WD1797::reset(void)
{
    trackReg_m        = 0;
    sectorReg_m       = 0;
    dataReg_m         = 0;
    cmdReg_m          = 0;
    statusReg_m       = 0;
    dataReady_m = false;
    intrqRaised_m = false;
    drqRaised_m = false;
    headLoaded_m = false;
    sectorLength_m = 0;
    lastIndexStatus_m = false;
    indexCount_m = 0;
    stepUpdate_m = false;
    stepSettle_m = 0;
    missCount_m = 0;
    seekSpeed_m       = 0;
    verifyTrack_m     = false;
    multiple_m        = false;
    delay_m           = false;
    side_m            = 0;
    deleteDAM_m       = false;
    state_m           = idleState;
    curCommand_m      = noneCmd;
    stepDirection_m   = dir_out;
    // leave curPos_m alone, diskette is still spinning...
    sectorPos_m       = -11;
}

BYTE
WD1797::in(BYTE addr)
{
    BYTE offset = addr - basePort_m;
    BYTE val    = 0;

    debugss(ssWD1797, ALL, "WD1797::in(%d)\n", addr);

    switch (offset)
    {
        case StatusPort_Offset_c:
            debugss(ssWD1797, INFO, "WD1797::in(StatusPort) (%02x) trk=%02x sec=%02x dat=%02x\n",
                    statusReg_m, trackReg_m, sectorReg_m, dataReg_m);
            val = statusReg_m;
            lowerIntrq();
            break;

        case TrackPort_Offset_c:
            debugss(ssWD1797, INFO, "WD1797::in(TrackPort)\n");
            val = trackReg_m;
            break;

        case SectorPort_Offset_c:
            debugss(ssWD1797, INFO, "WD1797::in(SectorPort)\n");
            val = sectorReg_m;
            break;

        case DataPort_Offset_c:
            val          = dataReg_m;
            dataReady_m  = false;
            statusReg_m &= ~stat_DataRequest_c;
            lowerDrq();
            break;

        default:
            debugss(ssWD1797, ERROR, "WD1797::in(Unknown - 0x%02x)\n", addr);
            break;

    }

    debugss_nts(ssWD1797, INFO, " - %d(0x%x)\n", val, val);
    return (val);
}

void
WD1797::out(BYTE addr,
            BYTE val)
{
    BYTE offset = addr - basePort_m;

    debugss(ssWD1797, ALL, "WD1797::out(%d, %d (0x%x))\n", addr, val, val);

    switch (offset)
    {
        case CommandPort_Offset_c:
            debugss(ssWD1797, INFO, "WD1797::out(CommandPort): %02x trk=%02x sec=%02x dat=%02x\n",
                    val, trackReg_m, sectorReg_m, dataReg_m);
            // MUST tolerate changing of selected disk *after* setting command...
            // timing is tricky...
            indexCount_m = 0;
            cmdReg_m     = val;
            processCmd(val);
            break;

        case TrackPort_Offset_c:
            debugss(ssWD1797, INFO, "WD1797::out(TrackPort): %d\n", val);
            trackReg_m = val;
            break;

        case SectorPort_Offset_c:
            debugss(ssWD1797, INFO, "WD1797::out(SectorPort): %d\n", val);
            sectorReg_m = val;
            break;

        case DataPort_Offset_c:
            debugss(ssWD1797, INFO, "WD1797::out(DataPort): %02x\n", val);

            // unpredictable results if !dataReady_m... (data changed while being written).
            // other mechanisms detect lostData, which is different.
            dataReg_m   = val;
            dataReady_m = true;
            lowerDrq();
            break;

        default:
            debugss(ssWD1797, ERROR, "WD1797::out(Unknown - 0x%02x): %d\n", addr, val);
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
    debugss(ssWD1797, INFO, "%s - cmd: 0x%02x\n", __FUNCTION__, cmd);

    // Make sure controller is not already busy. Documentation
    // did not specify what would happen, for now just ignore
    // the new command
    if ((statusReg_m & stat_Busy_c) == stat_Busy_c)
    {
        debugss(ssWD1797, WARNING, "New command while still busy: %d\n", cmd);
        // return;
    }

    // Can't reference drive here... it could be about to change.
    // GenericFloppyDrive *drive = getCurDrive();

    // First check for the Force Interrupt command
    if ((cmd & cmd_Mask_c) == cmd_ForceInterrupt_c)
    {
        processCmdTypeIV(cmd);
        return;
    }

    // set Busy flag
    statusReg_m |= stat_Busy_c;
    statusReg_m &= ~stat_LostData_c; // right?
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

    debugss(ssWD1797, INFO, "%s - cmd: %d\n", __FUNCTION__, cmd);
    // reset CRC Error, Seek Error, DRQ, INTRQ
    statusReg_m &= ~(stat_CRCError_c | stat_SeekError_c);
    lowerDrq();
    lowerIntrq();

    if ((cmd & 0xf0) == 0x00)
    {
        debugss(ssWD1797, INFO, "%s - Restore\n", __FUNCTION__);
        curCommand_m = restoreCmd;
    }
    else if ((cmd & 0xf0) == 0x10)
    {
        debugss(ssWD1797, INFO, "%s - Seek\n", __FUNCTION__);
        curCommand_m = dataReg_m == 0 ? restoreCmd : seekCmd;
    }
    else
    {
        // One of the Step commands
        debugss(ssWD1797, INFO, "%s - Step\n", __FUNCTION__);
        curCommand_m = stepCmd;

        stepUpdate_m = (cmd & cmdop_TrackUpdate_c) != 0;

        // check for step in or step out.
        if ((cmd & 0x40) == 0x40)
        {
            if ((cmd & 0x20) == 0x20)
            {
                // Step Out
                debugss(ssWD1797, INFO, "%s - Step Out\n", __FUNCTION__);
                stepDirection_m = dir_out;
            }
            else
            {
                // Step In
                debugss(ssWD1797, INFO, "%s - Step In\n", __FUNCTION__);
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

    debugss(ssWD1797, INFO, "%s - cmd: %d\n", __FUNCTION__, cmd);
    lowerDrq();
    dataReady_m = false;
    sectorPos_m = -11;

    if ((cmd & 0x20) == 0x20)
    {
        // Write Sector
        deleteDAM_m = ((cmd & cmdop_DataAddressMark_c) != 0);

        if (deleteDAM_m)
        {
            debugss(ssWD1797, WARNING, "%s - Deleted Data Addr Mark not supported - ignored\n",
                    __FUNCTION__);
        }

        debugss(ssWD1797, INFO, "%s - Write Sector - \n", __FUNCTION__);
        curCommand_m = writeSectorCmd;
    }
    else
    {
        // Read Sector
        debugss(ssWD1797, INFO, "%s - Read Sector: %d - multi: %d delay: %d sector: %d side: %d\n",
                __FUNCTION__, sectorReg_m, multiple_m, delay_m, sectorLength_m, side_m);
        curCommand_m = readSectorCmd;
    }

    stepSettle_m = 100; // give host time to get ready...
}

void
WD1797::processCmdTypeIII(BYTE cmd)
{
    delay_m     = ((cmd & cmdop_Delay_15ms_c) != 0);
    side_m      = ((cmd & cmdop_UpdateSSO_c) >> cmdop_UpdateSSO_Shift_c);
    loadHead(true);
    lowerDrq();
    dataReady_m = false;
    sectorPos_m = -11;

    debugss(ssWD1797, INFO, "%s - cmd: %d\n", __FUNCTION__, cmd);

    if ((cmd & 0xf0) == 0xc0)
    {
        // Read Address
        debugss(ssWD1797, INFO, "%s - Read Address\n", __FUNCTION__);
        curCommand_m = readAddressCmd;

    }
    else if ((cmd & 0xf0) == 0xf0)
    {
        // write Track
        debugss(ssWD1797, INFO, "%s - Write Track: %d\n", __FUNCTION__, trackReg_m);
        curCommand_m = writeTrackCmd;
        raiseDrq();

    }
    else if ((cmd & 0xf0) == 0xe0)
    {
        // read Track
        debugss(ssWD1797, INFO, "%s - Read Track: %d\n", __FUNCTION__, trackReg_m);
        curCommand_m = readTrackCmd;

    }
    else
    {
        debugss(ssWD1797, ERROR, "%s - Invalid type-III cmd: %x\n", __FUNCTION__, cmd);
        statusReg_m &= ~stat_Busy_c;
        return;
    }

    stepSettle_m = 100; // give host time to get ready...
}

// 'drive' might be NULL.
void
WD1797::processCmdTypeIV(BYTE cmd)
{
    debugss(ssWD1797, INFO, "%s - cmd: 0x%02x\n", __FUNCTION__, cmd);
    loadHead(false);
    // we assume drive won't change for Type IV commands...
    GenericFloppyDrive* drive = getCurDrive();

    curCommand_m = forceInterruptCmd;

    // check to see if previous command is still running
    if (statusReg_m & stat_Busy_c)
    {
        debugss(ssWD1797, INFO, "%s - Aborting Command\n", __FUNCTION__);
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
        debugss(ssWD1797, INFO, "%s - updating statusReg: %d\n", __FUNCTION__, statusReg_m);
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
            debugss(ssWD1797, INFO, "%s - Not Ready to Ready\n", __FUNCTION__);

        }

        if ((cmd & cmdop_ReadyToNotReady_c) == cmdop_ReadyToNotReady_c)
        {
            debugss(ssWD1797, INFO, "%s - Ready to Not Ready\n", __FUNCTION__);

        }

        if ((cmd & cmdop_IndexPulse_c) == cmdop_IndexPulse_c)
        {
            debugss(ssWD1797, INFO, "%s - Index Pulse\n", __FUNCTION__);

        }

        if ((cmd & cmdop_ImmediateInterrupt_c) == cmdop_ImmediateInterrupt_c)
        {
            debugss(ssWD1797, INFO, "%s - Immediate Interrupt\n", __FUNCTION__);
            statusReg_m &= ~stat_Busy_c;
            raiseIntrq();
        }
    }
    else
    {
        debugss(ssWD1797, INFO, "%s - No Interrupt/ Clear Busy\n", __FUNCTION__);
        statusReg_m &= ~stat_Busy_c;
        curCommand_m = noneCmd;
    }
}

void
WD1797::abortCmd()
{
    debugss(ssWD1797, INFO, "%s\n", __FUNCTION__);

    curCommand_m = noneCmd;
}

void
WD1797::raiseIntrq()
{
    debugss(ssWD1797, INFO, "%s\n", __FUNCTION__);
    intrqRaised_m = true;
}

void
WD1797::raiseDrq()
{
    debugss(ssWD1797, INFO, "%s\n", __FUNCTION__);
    drqRaised_m = true;
}

void
WD1797::lowerIntrq()
{
    debugss(ssWD1797, INFO, "%s\n", __FUNCTION__);
    intrqRaised_m = false;
}

void
WD1797::lowerDrq()
{
    debugss(ssWD1797, INFO, "%s\n", __FUNCTION__);
    drqRaised_m = false;
}

void
WD1797::loadHead(bool load)
{
    debugss(ssWD1797, INFO, "%s: %sload\n", __FUNCTION__, load ? "" : "un");
    headLoaded_m = load;
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
    unsigned long       charPos   = 0;
    GenericFloppyDrive* drive     = getCurDrive();
    bool                indexEdge = false;

    if (!drive)
    {
        // TODO: set some status indicator.
        statusReg_m |= stat_NotReady_c;

        if (curCommand_m != noneCmd)
        {
            abortCmd();
            // TODO: shouldn't abortCmd() do all this?
            raiseIntrq();
            statusReg_m &= ~stat_Busy_c;
        }

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
        // Position hasn't changed just return
        return;
    }

    debugss(ssWD1797, ALL, "New character Pos - old: %ld, new: %ld\n", curPos_m, charPos);
    curPos_m = charPos;

    switch (curCommand_m)
    {
        case restoreCmd:
        {
            if (!drive->getTrackZero())
            {
                drive->step(false);
                stepSettle_m = 100; // millisecToTicks(seekSpeed_m);
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
                stepSettle_m = 100; // millisecToTicks(seekSpeed_m);
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
                debugss(ssWD1797, INFO, "%s - step out\n", __FUNCTION__);

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

                    stepSettle_m = 100; // millisecToTicks(seekSpeed_m);

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
                debugss(ssWD1797, INFO, "%s - step in\n", __FUNCTION__);

                drive->step(true);
                statusReg_m &= ~stat_TrackZero_c;
                stepSettle_m = 100; // millisecToTicks(seekSpeed_m);

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

        default:
            debugss(ssWD1797, WARNING, "%s: default1: %d\n", __FUNCTION__,
                    curCommand_m);
            break;
    }

    int data;
    int result;

    switch (curCommand_m)
    {
        case restoreCmd:
        case seekCmd:
        case stepCmd:
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
            data        = drive->readData(
                doubleDensity(), trackReg_m, side_m, sectorReg_m, sectorPos_m);

            if (data == GenericFloppyFormat::NO_DATA)
            {
                // just wait for sector to come around..
            }
            else if (data == GenericFloppyFormat::DATA_AM)
            {
                sectorPos_m = 0;
            }

            else if (data == GenericFloppyFormat::CRC)
            {
                sectorPos_m  = -1;
                statusReg_m &= ~stat_Busy_c;
                raiseIntrq();
                curCommand_m = noneCmd;
            }

            else if (data < 0)
            {
                // probably ERROR
                sectorPos_m  = -1;
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
            data        = drive->readData(doubleDensity(), trackReg_m, side_m, 0xfd, sectorPos_m);

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
                sectorPos_m  = -1;
                statusReg_m &= ~stat_Busy_c;
                raiseIntrq();
                curCommand_m = noneCmd;
            }

            else if (data < 0)
            {
                // probably ERROR
                sectorPos_m  = -1;
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
                sectorPos_m  = -1;
                statusReg_m &= ~stat_Busy_c;
                raiseIntrq();
                curCommand_m = noneCmd;
            }

            else if (result < 0)
            {
                // other errors
                sectorPos_m  = -1;
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
            data        = drive->readData(doubleDensity(), trackReg_m, side_m, 0xff, sectorPos_m);

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
                sectorPos_m  = -1;
                statusReg_m &= ~stat_Busy_c;
                raiseIntrq();
                curCommand_m = noneCmd;
            }

            else if (data < 0)
            {
                // probably ERROR
                sectorPos_m  = -1;
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
                sectorPos_m  = -1;
                statusReg_m &= ~stat_Busy_c;
                raiseIntrq();
                curCommand_m = noneCmd;
            }

            else if (result < 0)
            {
                // other errors
                debugss(ssWD1797, ERROR, "Error in write track\n");
                sectorPos_m  = -1;
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
