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
                              cmdIV_readyToNotReady(false),
                              cmdIV_notReadyToReady(false),
                              cmdIV_indexPulse(false),
                              userIf_m(nullptr),
                              curNotification(&WD1797::noneNotification),
                              currentDrive_m(nullptr),
                              cycleCount_m(0),
                              formattingState_m(fs_none),
                              doubleDensity_m(false),
                              immediateInterruptSet_m(false)
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
    trackReg_m              = 0;
    sectorReg_m             = 0;
    dataReg_m               = 0;
    cmdReg_m                = 0;
    statusReg_m             = 0;
    dataReady_m             = false;
    intrqRaised_m           = false;
    drqRaised_m             = false;
    headLoaded_m            = false;
    sectorLength_m          = 0;
    lastIndexStatus_m       = false;
    indexCount_m            = 0;
    stepUpdate_m            = false;
    stepSettle_m            = 0;
    missCount_m             = 0;
    seekSpeed_m             = 0;
    verifyTrack_m           = false;
    multiple_m              = false;
    delay_m                 = false;
    side_m                  = 0;
    deleteDAM_m             = false;
    curCommand_m            = noneCmd;
    cmdIV_readyToNotReady   = false;
    cmdIV_notReadyToReady   = false;
    cmdIV_indexPulse        = false;
    immediateInterruptSet_m = false;

    curNotification         = &WD1797::noneNotification;
    stepDirection_m         = dir_out;
    // leave curPos_m alone, diskette is still spinning...
    sectorPos_m             = InitialSectorPos_c;
}


BYTE
WD1797::in(BYTE addr)
{
    BYTE offset = addr - basePort_m;
    BYTE val    = 0;

    debugss(ssWD1797, INFO, "(%d)\n", addr);

    switch (offset)
    {
        case StatusPort_Offset_c:
            debugss(ssWD1797, INFO, "(StatusPort) (0x%02x) trk=%d sec=%d dat=0x%02x\n",
                    statusReg_m, trackReg_m, sectorReg_m, dataReg_m);

            val = statusReg_m;
            if (!immediateInterruptSet_m)
            {
                lowerIntrq();
            }
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

    debugss(ssWD1797, INFO, "(%d, %d (0x%x))\n", addr, val, val);

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
            debugss(ssWD1797, INFO, "(TrackPort): %d\n", val);
            trackReg_m = val;
            break;

        case SectorPort_Offset_c:
            debugss(ssWD1797, VERBOSE, "(SectorPort): %d\n", val);
            sectorReg_m = val;
            break;

        case DataPort_Offset_c:
            debugss(ssWD1797, INFO, "(DataPort): %02x\n", val);

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
        debugss(ssWD1797, ERROR, "New command while still busy: %d\n", cmd);
        // TODO determine the proper behavior.
        // return;
    }

    // set Busy flag
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
    debugss(ssWD1797, INFO, "cmd: 0x%02x\n", cmd);

    verifyTrack_m = ((cmd & cmdop_VerifyTrack_c) != 0);
    seekSpeed_m   = speeds[cmd & cmdop_StepMask_c];
    dataReady_m   = false;

    if (userIf_m->getClockPeriod() > 500)
    {
        seekSpeed_m *= 2;
    }

    loadHead((cmd & cmdop_HeadLoad_c) != 0);
    stepUpdate_m = true;

    // reset CRC Error, Seek Error, DRQ, INTRQ

    statusReg_m &= ~(stat_CRCError_c | stat_SeekError_c);
    lowerDrq();
    if (!immediateInterruptSet_m)
    {
        lowerIntrq();
    }

    if ((cmd & 0xf0) == 0x00)
    {
        debugss(ssWD1797, WARNING, "Restore: %dms, v=%d\n", seekSpeed_m, verifyTrack_m);

        curCommand_m = restoreCmd;
        trackReg_m   = 255;
    }
    else if ((cmd & 0xe0) == 0x00)
    {
        // Some of the documentation shows two different bit patterns for seek,
        // but none for plain 'step', this is an apparent typo, the version of
        // the document in the Z-89-37 seems to be the correct one and the
        // pattern "0 0 1 T h V r1 r2" is the step command not the seek command.

        debugss(ssWD1797, WARNING, "Seek: %d, %dms, v=%d\n", dataReg_m, seekSpeed_m, verifyTrack_m);
        curCommand_m = seekCmd;
    }
    else
    {
        // One of the Step commands
        stepUpdate_m = (cmd & cmdop_TrackUpdate_c) != 0;
        debugss(ssWD1797, WARNING, "Step: %dms, v=%d ut: %d\n", seekSpeed_m, verifyTrack_m,
                stepUpdate_m);
        curCommand_m = stepCmd;

        stepUpdate_m = (cmd & cmdop_TrackUpdate_c) != 0;

        // check for step in or step out.
        if ((cmd & 0x40) == 0x40)
        {
            if ((cmd & 0x20) == 0x20)
            {
                // Step Out
                debugss(ssWD1797, WARNING, "Step Out\n");
                stepDirection_m = dir_out;
            }
            else
            {
                // Step In
                debugss(ssWD1797, WARNING, "Step In\n");
                stepDirection_m = dir_in;
            }
        }
    }

    // Drive selection might change, need to delay start of command...
    stepSettle_m    = HeadSettleTimeInTicks_c;
    curNotification = &WD1797::cmdTypeI_Notification;
}

void
WD1797::processCmdTypeII(BYTE cmd)
{
    multiple_m     = ((cmd & cmdop_MultipleRecord_c) != 0);
    delay_m        = ((cmd & cmdop_Delay_15ms_c) != 0);
    sectorLength_m = ((cmd & cmdop_SectorLength_c) != 0 ? 1 : 0);
    side_m         = ((cmd & cmdop_UpdateSSO_c) >> cmdop_UpdateSSO_Shift_c);
    loadHead(true);

    debugss(ssWD1797, INFO, "cmd: 0x%02x\n", cmd);
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

    stepSettle_m    = HeadSettleTimeInTicks_c; // give host time to get ready...
    curNotification = &WD1797::cmdTypeII_Notification;
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

    debugss(ssWD1797, INFO, "cmd: 0x%02x\n", cmd);

    if ((cmd & 0xf0) == 0xc0)
    {
        // Read Address
        debugss(ssWD1797, INFO, "Read Address\n");

        curCommand_m = readAddressCmd;
    }
    else if ((cmd & 0xf0) == 0xf0)
    {
        // write Track
        debugss(ssWD1797, ERROR, "Write Track: %d\n", trackReg_m);

        currentDrive_m->startTrackFormat(trackReg_m);
        curCommand_m      = writeTrackCmd;
        formattingState_m = fs_waitingForIndex;
        raiseDrq();
    }
    else if ((cmd & 0xf0) == 0xe0)
    {
        // read Track
        debugss(ssWD1797, ERROR, "Read Track: %d\n", trackReg_m);
        curCommand_m = readTrackCmd;
    }
    else
    {
        debugss(ssWD1797, ERROR, "Invalid type-III cmd: %x\n", cmd);
        statusReg_m &= ~stat_Busy_c;
        return;
    }

    stepSettle_m    = HeadSettleTimeInTicks_c; // give host time to get ready...
    curNotification = &WD1797::cmdTypeIII_Notification;
}

// 'drive' might be NULL.
void
WD1797::processCmdTypeIV(BYTE cmd)
{
    debugss(ssWD1797, INFO, "cmd-IV: 0x%02x\n", cmd);

    loadHead(false);
    // we assume drive won't change for Type IV commands...

    curCommand_m = forceInterruptCmd;

    // check to see if previous command is still running
    if (statusReg_m & stat_Busy_c)
    {
        debugss(ssWD1797, INFO, "Aborting Command\n");
        // still running, abort command and reset busy
        abortCmd();
    }
    else if (currentDrive_m)
    {
        // no Command running, update status.
        // try more-surgical repair of status register.
        statusReg_m &= ~stat_SeekError_c;
        statusReg_m &= ~stat_CRCError_c;
        // if previous command was Type II or III, need to clear
        // stat_LostData_c and stat_DataRequest_c ? But not if Type I.
        debugss(ssWD1797, INFO, "updating statusReg: %d\n", statusReg_m);
    }
    else
    {
        debugss(ssWD1797, WARNING, "Type IV with no drive\n");
        statusReg_m = stat_NotReady_c;
    }

    if ((cmd & 0x0f) != 0x00)
    {
        // at least one bit is set.
        cmdIV_notReadyToReady = ((cmd & cmdop_NotReadyToReady_c) == cmdop_NotReadyToReady_c);
        debugss(ssWD1797, INFO, "Not Ready to Ready - %d\n", cmdIV_notReadyToReady);

        cmdIV_readyToNotReady = ((cmd & cmdop_ReadyToNotReady_c) == cmdop_ReadyToNotReady_c);
        debugss(ssWD1797, INFO, "Ready to Not Ready - %d\n", cmdIV_readyToNotReady);

        cmdIV_indexPulse      = ((cmd & cmdop_IndexPulse_c) == cmdop_IndexPulse_c);
        debugss(ssWD1797, INFO, "Index Pulse - %d\n", cmdIV_indexPulse);

        if ((cmd & cmdop_ImmediateInterrupt_c) == cmdop_ImmediateInterrupt_c)
        {
            debugss(ssWD1797, INFO, "Immediate Interrupt\n");
            statusReg_m            &= ~stat_Busy_c;
            immediateInterruptSet_m = true;
            raiseIntrq();
        }
        curNotification = &WD1797::cmdTypeIV_Notification;
    }
    else
    {
        debugss(ssWD1797, INFO, "No Interrupt/ Clear Busy\n");
        statusReg_m            &= ~stat_Busy_c;
        curCommand_m            = noneCmd;
        curNotification         = &WD1797::noneNotification;
        immediateInterruptSet_m = false;
        lowerIntrq();
        updateStatusTypeI(currentDrive_m);
    }
}

void
WD1797::verifyTrack(GenericFloppyDrive* drive)
{
    int track, sector, side;

    if (drive)
    {
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
    else
    {
        debugss(ssWD1797, INFO, "seekCmd no drive: %d\n", trackReg_m);
        statusReg_m |= stat_SeekError_c;
    }
}

void
WD1797::completeCmd()
{
    debugss(ssWD1797, INFO, "\n");

    curCommand_m      = noneCmd;
    statusReg_m      &= ~stat_Busy_c;
    formattingState_m = fs_none;
    raiseIntrq();
    curNotification   = &WD1797::noneNotification;
}

void
WD1797::abortCmd()
{
    debugss(ssWD1797, INFO, "\n");

    curCommand_m      = noneCmd;
    statusReg_m      &= ~stat_Busy_c;
    formattingState_m = fs_none;
    curNotification   = &WD1797::noneNotification;
}

void
WD1797::raiseIntrq()
{
    debugss(ssWD1797, INFO, "\n");

    intrqRaised_m = true;
    userIf_m->raiseIntrq();
}

void
WD1797::raiseDrq()
{
    debugss(ssWD1797, INFO, "\n");

    drqRaised_m = true;
    userIf_m->raiseDrq();
}

void
WD1797::lowerIntrq()
{
    debugss(ssWD1797, INFO, "\n");

    intrqRaised_m = false;
    userIf_m->lowerIntrq();
}

void
WD1797::lowerDrq()
{
    debugss(ssWD1797, INFO, "\n");

    drqRaised_m = false;
    userIf_m->lowerDrq();
}


void
WD1797::loadHead(bool load)
{
    debugss(ssWD1797, INFO, "%sload\n", load ? "" : "un");
    headLoaded_m = load;

    if (currentDrive_m)
    {
        currentDrive_m->headLoad(load);
    }
}

/// data transfered from the drive to the controller chip.
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


void
WD1797::notification(unsigned int cycleCount)
{
    cycleCount_m += cycleCount;

    (this->*curNotification)(cycleCount);
}

// new notification function
void
WD1797::noneNotification(unsigned int cycleCount)
{
    if (userIf_m->readReady())
    {
        // Clear notReady
        statusReg_m &= ~stat_NotReady_c;
    }
    else
    {
        // Set notReady
        statusReg_m |= stat_NotReady_c;
    }

    if (currentDrive_m)
    {
        if (currentDrive_m->isWriteProtect())
        {
            statusReg_m |= stat_WriteProtect_c;
        }
        else
        {
            statusReg_m &= ~stat_WriteProtect_c;
        }
        if (currentDrive_m->getIndexPulse())
        {
            statusReg_m |= stat_IndexPulse_c;
        }
        else
        {
            statusReg_m &= ~stat_IndexPulse_c;
        }
    }
}

void
WD1797::cmdTypeI_Notification(unsigned int cycleCount)
{

    if (userIf_m->readReady())
    {
        statusReg_m &= ~stat_NotReady_c;
    }
    else
    {
        statusReg_m |= stat_NotReady_c;
    }

    if (currentDrive_m && currentDrive_m->getIndexPulse())
    {
        statusReg_m |= stat_IndexPulse_c;
        if (!lastIndexStatus_m)
        {
            ++indexCount_m;
        }

        lastIndexStatus_m = true;
    }
    else
    {
        statusReg_m      &= ~stat_IndexPulse_c;
        lastIndexStatus_m = false;
    }

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

    switch (curCommand_m)
    {
        case restoreCmd:
            if (currentDrive_m && currentDrive_m->getTrackZero())
            {
                trackReg_m   = 0;
                statusReg_m |= stat_TrackZero_c;
                if (verifyTrack_m)
                {
                    verifyTrack(currentDrive_m);
                }

                completeCmd();
            }
            else
            {
                if (trackReg_m)
                {
                    --trackReg_m;
                    if (currentDrive_m)
                    {
                        currentDrive_m->step(false);
                    }

                    stepSettle_m = HeadSettleTimeInTicks_c;
                }
                else
                {
                    // went 255 steps, still not seeing track 0.
                    if (verifyTrack_m)
                    {
                        statusReg_m |= stat_SeekError_c;
                    }
                    completeCmd();
                }
            }
            break;

        case seekCmd:
            if (dataReg_m != trackReg_m)
            {
                bool dir = (dataReg_m > trackReg_m);
                if (currentDrive_m)
                {
                    currentDrive_m->step(dir);
                }
                trackReg_m  += (dir ? 1 : -1);
                stepSettle_m = HeadSettleTimeInTicks_c;
            }
            else
            {
                if (verifyTrack_m)
                {
                    verifyTrack(currentDrive_m);
                }

                if (currentDrive_m && currentDrive_m->getTrackZero())
                {
                    statusReg_m |= stat_TrackZero_c;
                }
                else
                {
                    statusReg_m &= ~stat_TrackZero_c;
                }
                completeCmd();
            }
            break;

        case stepCmd:
            if (stepDirection_m == dir_out)
            {
                debugss(ssWD1797, INFO, "step out\n");

                if (currentDrive_m && currentDrive_m->getTrackZero())
                {
                    statusReg_m |= stat_TrackZero_c;
                }
                else
                {
                    if (currentDrive_m)
                    {
                        currentDrive_m->step(false);
                    }

                    stepSettle_m = HeadSettleTimeInTicks_c;

                    if (stepUpdate_m)
                    {
                        trackReg_m--;
                    }
                }
            }
            else if (stepDirection_m == dir_in)
            {
                debugss(ssWD1797, INFO, "step in\n");
                if (currentDrive_m)
                {
                    currentDrive_m->step(true);
                }
                statusReg_m &= ~stat_TrackZero_c;
                stepSettle_m = HeadSettleTimeInTicks_c;

                if (stepUpdate_m)
                {
                    trackReg_m++;
                }
            }

            curCommand_m = stepDoneCmd;
            break;

        case stepDoneCmd:
            if (currentDrive_m && currentDrive_m->getTrackZero())
            {
                statusReg_m |= stat_TrackZero_c;
            }
            else
            {
                statusReg_m &= ~stat_TrackZero_c;
            }

            completeCmd();
            break;

        default:
            debugss(ssWD1797, ERROR, "unexpect command: %d\n", curCommand_m);
            break;
    }

}

void
WD1797::cmdTypeII_Notification(unsigned int cycleCount)
{
    if (userIf_m->readReady())
    {
        // Clear notReady
        statusReg_m &= ~stat_NotReady_c;
    }
    else
    {
        // Set notReady
        statusReg_m |= stat_NotReady_c;

        abortCmd();
        raiseIntrq();
        return;
    }

    if (currentDrive_m)
    {
        if (currentDrive_m->getIndexPulse())
        {
            if (!lastIndexStatus_m)
            {
                ++indexCount_m;
            }

            lastIndexStatus_m = true;
        }
        else
        {
            lastIndexStatus_m = false;
        }
    }

    if (stepSettle_m > 0)
    {

        if (stepSettle_m > cycleCount)
        {
            stepSettle_m -= cycleCount;
            return;
        }

        stepSettle_m = 0;
        missCount_m  = 0;
    }

    if (!currentDrive_m)
    {
        debugss(ssWD1797, WARNING, "No drive\n");
        // TODO determine right way to handle this... on MMS controller, ready would
        // not be set, so it would abort above, abort for now on the H37
        abortCmd();
        raiseIntrq();

        return;
    }

    unsigned long charPos = currentDrive_m->getCharPos(doubleDensity_m);

    if (charPos == curPos_m)
    {
        // Position hasn't changed just return
        return;
    }
    debugss(ssWD1797, ALL, "New character Pos - old: %ld, new: %ld\n", curPos_m, charPos);
    curPos_m = charPos;

    int data;
    int result;

    switch (curCommand_m)
    {
        case readSectorCmd:
            debugss(ssWD1797, INFO, "readSectorCmd - dataReady_m: %d\n", dataReady_m);
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
            currentDrive_m->selectSide(side_m);
            debugss(ssWD1797, WARNING, "track: %d, sector: %d, sectorPos_m- %d\n", trackReg_m,
                    sectorReg_m, sectorPos_m);

            // only check for correct sector at the start of the sector.
            if (sectorPos_m == InitialSectorPos_c &&
                !currentDrive_m->verifyTrackSector(trackReg_m, sectorReg_m))
            {
                debugss(ssWD1797, WARNING, "sector not found track: %d, sector: %d\n", trackReg_m,
                        sectorReg_m);

                statusReg_m |= stat_RecordNotFound_c;

                completeCmd();
                return;
            }

            data = currentDrive_m->readData(doubleDensity_m, trackReg_m,
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
                if (multiple_m)
                {
                    sectorReg_m++;
                    sectorPos_m = InitialSectorPos_c;
                }
                else
                {
                    sectorPos_m = ErrorSectorPos_c;
                    completeCmd();
                }
            }
            else if (data < 0)
            {
                debugss(ssWD1797, WARNING, "less than zero\n");
                // probably ERROR
                sectorPos_m  = ErrorSectorPos_c;
                statusReg_m |= stat_CRCError_c;
                completeCmd();
            }
            else
            {
                transferData(data);
                ++sectorPos_m;
            }
            break;

        case writeSectorCmd:

            currentDrive_m->selectSide(side_m);

            // only check for correct sector at the start of the sector.
            if (sectorPos_m == InitialSectorPos_c &&
                !currentDrive_m->verifyTrackSector(trackReg_m, sectorReg_m))
            {
                statusReg_m |= stat_RecordNotFound_c;
                completeCmd();
                return;
            }


            // TODO - dataReady shouldn't be sent to the drive, according to the
            // WD docs, after the start of writing, if a DRQ is not responded to, it
            // should write '0' and set the status flag - LOST DATA.
            result = currentDrive_m->writeData(doubleDensity_m, trackReg_m, side_m, sectorReg_m,
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
                if (multiple_m)
                {
                    sectorReg_m++;
                    sectorPos_m = InitialSectorPos_c;
                }
                else
                {
                    sectorPos_m = ErrorSectorPos_c;
                    completeCmd();
                }
            }
            else if (result < 0)
            {
                // other errors
                sectorPos_m  = ErrorSectorPos_c;
                statusReg_m |= stat_WriteFault_c;
                completeCmd();
            }
            else
            {
                dataReady_m = false;
                ++sectorPos_m;
                raiseDrq();
            }
            break;

        default:
            debugss(ssWD1797, ERROR, "Unexpected cmd: %d\n", curCommand_m);
            break;

    }
}


void
WD1797::cmdTypeIII_Notification(unsigned int cycleCount)
{
    if (userIf_m->readReady())
    {
        // Clear notReady
        statusReg_m &= ~stat_NotReady_c;
    }
    else
    {
        // Set notReady
        statusReg_m |= stat_NotReady_c;

        abortCmd();
        raiseIntrq();

        return;
    }

    if (currentDrive_m)
    {
        if (currentDrive_m->getIndexPulse())
        {
            if (!lastIndexStatus_m)
            {
                lastIndexStatus_m = true;
                ++indexCount_m;
                if (formattingState_m == fs_waitingForIndex)
                {
                    formattingState_m = fs_writing;
                }
            }
        }
        else
        {
            lastIndexStatus_m = false;
        }
    }

    if (stepSettle_m > 0)
    {
        if (stepSettle_m > cycleCount)
        {
            stepSettle_m -= cycleCount;
            return;
        }

        stepSettle_m = 0;
        missCount_m  = 0;
    }

    if (!currentDrive_m)
    {
        debugss(ssWD1797, WARNING, "No drive\n");
        // TODO figure out the right thing to do
        abortCmd();
        raiseIntrq();

        return;
    }

    unsigned long charPos = currentDrive_m->getCharPos(doubleDensity_m);

    if (charPos == curPos_m)
    {
        // Position hasn't changed just return
        return;
    }

    debugss(ssWD1797, ALL, "New character Pos - old: %ld, new: %ld\n", curPos_m, charPos);
    curPos_m = charPos;

    int data;
    int result;

    switch (curCommand_m)
    {
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
            currentDrive_m->selectSide(side_m);
            // sector '0xfd' indicates a read address
            data        = currentDrive_m->readData(doubleDensity_m, trackReg_m, side_m,
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
                sectorPos_m = ErrorSectorPos_c;
                completeCmd();
            }
            else if (data < 0)
            {
                // probably ERROR
                sectorPos_m  = ErrorSectorPos_c;
                statusReg_m |= stat_CRCError_c;
                completeCmd();
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
            currentDrive_m->selectSide(side_m);
            data        = currentDrive_m->readData(doubleDensity_m, trackReg_m, side_m,
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
                sectorPos_m = ErrorSectorPos_c;
                completeCmd();
            }
            else if (data < 0)
            {
                // probably ERROR
                sectorPos_m  = ErrorSectorPos_c;
                statusReg_m |= stat_CRCError_c;
                completeCmd();
            }
            else
            {
                transferData(data);
                ++sectorPos_m;
            }
            break;

        case writeTrackCmd:
            currentDrive_m->selectSide(side_m);

            // \todo
            result = currentDrive_m->writeData(doubleDensity_m, trackReg_m, side_m, 0xff,
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
                sectorPos_m = ErrorSectorPos_c;
                completeCmd();
            }
            else if (result < 0)
            {
                // other errors
                debugss(ssWD1797, WARNING, "Error in write track\n");
                sectorPos_m  = ErrorSectorPos_c;
                statusReg_m |= stat_WriteFault_c;
                completeCmd();
            }
            else
            {
                dataReady_m = false;
                ++sectorPos_m;
                raiseDrq();
            }
            break;

        default:
            debugss(ssWD1797, ERROR, "Unexpected cmd: %d\n", curCommand_m);
            break;

    }
}

void
WD1797::cmdTypeIV_Notification(unsigned int cycleCount)
{
    // TODO - determine if once condition is met, does the command terminate.

    if (userIf_m->readReady())
    {
        if (cmdIV_notReadyToReady && ((statusReg_m & stat_NotReady_c) == 1))
        {
            raiseIntrq();
        }
        // Clear notReady
        statusReg_m &= ~stat_NotReady_c;
    }
    else
    {
        if (cmdIV_readyToNotReady && ((statusReg_m & stat_NotReady_c) == 0))
        {
            raiseIntrq();
        }
        // Set notReady
        statusReg_m |= stat_NotReady_c;
    }

    if (currentDrive_m)
    {
        if (currentDrive_m->getIndexPulse())
        {
            if (!lastIndexStatus_m)
            {
                ++indexCount_m;
                if (cmdIV_indexPulse)
                {
                    raiseIntrq();
                }
            }

            lastIndexStatus_m = true;
        }
        else
        {
            lastIndexStatus_m = false;
        }
    }
}


void
WD1797::updateStatusTypeI(GenericFloppyDrive* drive)
{
    statusReg_m = 0;
    if (!userIf_m->readReady())
    {
        statusReg_m |= stat_NotReady_c;
    }
    else
    {
        statusReg_m &= ~stat_NotReady_c;
    }

    if (drive)
    {
        if (drive->isWriteProtect())
        {
            statusReg_m |= stat_WriteProtect_c;
        }
        else
        {
            statusReg_m &= ~stat_WriteProtect_c;
        }
        /// \todo head load status
        if (drive->getTrackZero())
        {
            statusReg_m |= stat_TrackZero_c;
        }
        else
        {
            statusReg_m &= ~stat_TrackZero_c;
        }
        if (drive->getIndexPulse())
        {
            statusReg_m |= stat_IndexPulse_c;
        }
        else
        {
            statusReg_m &= ~stat_IndexPulse_c;
        }
    }
}


void
WD1797::setDoubleDensity(bool dd)
{
    doubleDensity_m = dd;
}

unsigned long
WD1797::millisecToTicks(unsigned long ms)
{
    unsigned long tps   = WallClock::instance()->getTicksPerSecond();
    unsigned long ticks = (tps * ms) / 1000;
    return ticks;
}

void
WD1797::setCurrentDrive(GenericFloppyDrive* drive)
{
    currentDrive_m = drive;
}
