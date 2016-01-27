/// \file h37.cpp
///
/// \date May 13, 2009
/// \author Mark Garlanger
///

#include "H89.h"
#include "h37.h"
#include "logger.h"
#include "DiskDrive.h"

const BYTE Z_89_37::speeds[maxStepSpeeds_c] = { 6, 12, 20, 30};

Z_89_37::Z_89_37(int baseAddr): IODevice(baseAddr, H37_NumPorts_c),
	                            trackReg_m(0),
	                            sectorReg_m(0),
	                            dataReg_m(0),
	                            cmdReg_m(0),
	                            statusReg_m(0),
	                            interfaceReg_m(0),
	                            controlReg_m(0),
                                motorOn_m(false),
                                dataReady_m(false),
                                lostDataStatus_m(false),
	                            seekSpeed_m(0),
	                            verifyTrack_m(false),
	                            multiple_m(false),
	                            delay_m(false),
	                            sectorLength_m(0),
	                            side_m(0),
	                            deleteDAM_m(false),
	                            state_m(idleState),
	                            curCommand_m(noneCmd),
	                            sectorTrackAccess_m(false),
	                            dataEncoding_m(FM),
	                            stepDirection_m(dir_out),
	                            curDiskDrive_m(numDisks_c),
	                            intLevel_m(z_89_37_Intr_c),
	                            curPos_m(0),
	                            sectorPos_m(-10),
                                intrqAllowed_m(false),
	                            drqAllowed_m(false),
	                            cycleCount_m(0)
{
    drives_m[ds0] = drives_m[ds1] = drives_m[ds2] = drives_m[ds3] = nullptr;
    WallClock::instance()->registerUser(this);
}

Z_89_37::~Z_89_37()
{
    WallClock::instance()->unregisterUser(this);
}

void Z_89_37::reset(void)
{
    trackReg_m          = 0;
    sectorReg_m         = 0;
    dataReg_m           = 0;
    cmdReg_m            = 0;
    statusReg_m         = 0;
    interfaceReg_m      = 0;
    controlReg_m        = 0;
    seekSpeed_m         = 0;
    verifyTrack_m       = false;
	multiple_m          = false;
	delay_m             = false;
	sectorLength_m      = 0;
	side_m              = 0;
    deleteDAM_m         = false;
    state_m             = idleState;
    curCommand_m        = noneCmd;
    sectorTrackAccess_m = false;
    dataEncoding_m      = FM;
    stepDirection_m     = dir_out;
    curDiskDrive_m      = numDisks_c;
    intLevel_m          = z_89_37_Intr_c;
    intrqAllowed_m      = false;
    drqAllowed_m        = false;
    cycleCount_m        = 0;

    motorOn_m           = false;
    dataReady_m         = false;
    lostDataStatus_m    = false;

    drives_m[ds0]       = drives_m[ds1]
                        = drives_m[ds2]
                        = drives_m[ds3]
                        = nullptr;
}

BYTE Z_89_37::in(BYTE addr)
{
	BYTE offset = getPortOffset(addr);
    BYTE val = 0;

    debugss(ssH37, ALL, "H37::in(%d)\n", addr);

    switch (offset)
    {
    case ControlPort_Offset_c:
        debugss(ssH37, INFO, "H37::in(ControlPort)");
        val = controlReg_m;
        break;

    case InterfaceControl_Offset_c:
        debugss(ssH37, INFO, "H37::in(InterfaceControl)");
        if (sectorTrackAccess_m)
        {
            val = interfaceReg_m;
        }
        break;

    case StatusPort_Offset_c:
        if (sectorTrackAccess_m)
        {
            debugss(ssH37, INFO, "H37::in(SectorPort)");
            val = sectorReg_m;
        }
        else
        {
            debugss(ssH37, INFO, "H37::in(StatusPort)");
            val = statusReg_m;
            lowerIntrq();
        }
        break;

    case DataPort_Offset_c:
        if (sectorTrackAccess_m)
        {
            debugss(ssH37, INFO, "H37::in(TrackPort)");
            val = trackReg_m;
        }
        else
        {
            debugss(ssH37, INFO, "H37::in(DataPort)");
            val = dataReg_m;

        }
        break;

    default:
        debugss(ssH37, ERROR, "H37::in(Unknown - 0x%02x)", addr);
    	break;

    }
    debugss_nts(ssH37, INFO, " - %d(0x%x)\n", val, val);
    return(val);
}

void Z_89_37::out(BYTE addr, BYTE val)
{
	BYTE offset = getPortOffset(addr);

	debugss(ssH37, ALL, "H37::out(%d, %d (0x%x))\n", addr, val, val);
    switch (offset)
    {
    case ControlPort_Offset_c:
        debugss(ssH37, INFO, "H37::out(ControlPort)");
        controlReg_m = val;
        if (val & ctrl_EnableIntReq_c)
        {
            debugss_nts(ssH37, INFO, " EnableIntReq");
            intrqAllowed_m = true;
        }
        else
        {
            debugss_nts(ssH37, INFO, " DisableIntReq");
            intrqAllowed_m = false;
        }
        if (val & ctrl_EnableDrqInt_c)
        {
            debugss_nts(ssH37, INFO, " EnableDrqInt");
            drqAllowed_m = true;
        }
        else
        {
            debugss_nts(ssH37, INFO, " DisableDrqReq");
            drqAllowed_m = false;
        }
        if (val & ctrl_SetMFMRecording_c)
        {
            debugss_nts(ssH37, INFO, " SetMFM");
            dataEncoding_m = MFM;
        }
        else
        {
        	dataEncoding_m = FM;
        }
        if (val & ctrl_MotorsOn_c)
        {
            debugss_nts(ssH37, INFO, " MotorsOn");
            motorOn_m = true;
        }
        else
        {
            motorOn_m = false;
        }
        if (val & ctrl_Drive_0_c)
        {
            debugss_nts(ssH37, INFO, " Drive0");
            curDiskDrive_m = ds0;
        }
        if (val & ctrl_Drive_1_c)
        {
            debugss_nts(ssH37, INFO, " Drive1");
            curDiskDrive_m = ds1;
        }
        if (val & ctrl_Drive_2_c)
        {
            debugss_nts(ssH37, INFO, " Drive2");
            curDiskDrive_m = ds2;
        }
        if (val & ctrl_Drive_3_c)
        {
            debugss_nts(ssH37, INFO, " Drive3");
            curDiskDrive_m = ds3;
        }
        debugss_nts(ssH37, INFO,"\n");
        break;

    case InterfaceControl_Offset_c:
        debugss(ssH37, INFO, "H37::out(InterfaceControl) - ");

        interfaceReg_m = val;
        if ((val & if_SelectSectorTrack_c) == if_SelectSectorTrack_c)
        {
            debugss_nts(ssH37, INFO, "Sector/Track Access\n");
            sectorTrackAccess_m = true;
        }
        else
        {
            debugss_nts(ssH37, INFO, "Control/Data Access\n");
            sectorTrackAccess_m = false;
        }
        break;

    case CommandPort_Offset_c:
        if (sectorTrackAccess_m)
        {
            debugss(ssH37, INFO, "H37::out(SectorPort): %d\n", val);
            sectorReg_m = val;
        }
        else
        {
            debugss(ssH37, INFO, "H37::out(CommandPort): %d\n", val);
            cmdReg_m = val;
            processCmd(val);

        }
        break;

    case DataPort_Offset_c:
        if (sectorTrackAccess_m)
        {
            debugss(ssH37, INFO, "H37::out(TrackPort): %d\n", val);
            trackReg_m = val;
        }
        else
        {
            debugss(ssH37, INFO, "H37::out(DataPort): %d\n", val);
            dataReg_m = val;

            dataReady_m = false;
            lowerDrq();
        }
        break;

    default:
        debugss(ssH37, ERROR, "H37::out(Unknown - 0x%02x): %d\n", addr, val);
    	break;
    }
}

void Z_89_37::processCmd(BYTE cmd)
{
    debugss(ssH37, INFO, "%s - cmd: 0x%02x\n", __FUNCTION__, cmd);
	// First check for the Force Interrupt command
	if ((cmd & cmd_Mask_c) == cmd_ForceInterrupt_c)
	{
		processCmdTypeIV(cmd);
		return;
	}
	// Make sure controller is not already busy. Documentation
	// did not specify what would happen, for now just ignore
	// the new command
	if ((statusReg_m & stat_Busy_c) == stat_Busy_c)
	{
		debugss(ssH37, WARNING, "New command while still busy: %d", cmd);
		//return;
	}

	// set Busy flag
	statusReg_m |= stat_Busy_c;
	debugss(ssH37, INFO, "Setting busy flag\n")

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

void Z_89_37::processCmdTypeI(BYTE cmd)
{
    verifyTrack_m = ((cmd & cmdop_VerifyTrack_c) == cmdop_VerifyTrack_c);
    seekSpeed_m = speeds[cmd & cmdop_StepMask_c];

    debugss(ssH37, INFO, "%s - cmd: %d\n", __FUNCTION__, cmd);
    // reset CRC Error, Seek Error, DRQ, INTRQ
    statusReg_m &= ~(stat_CRCError_c | stat_SeekError_c);
    lowerDrq();
    lowerIntrq();

    if ((cmd & cmdop_HeadLoad_c) == cmdop_HeadLoad_c)
    {
        debugss(ssH37, INFO, "%s - Load head at beginning\n", __FUNCTION__);
    	loadHead();
    }
    else
    {
        debugss(ssH37, INFO, "%s - Unload head at beginning\n", __FUNCTION__);
    	unloadHead();
    }

    if ((cmd & 0xe0) == 0x00)
    {
	    debugss(ssH37, INFO, "%s - Seek\n", __FUNCTION__);
    	if ((cmd & 0x10) != 0x10)
    	{
    	    debugss(ssH37, INFO, "%s - Restore\n", __FUNCTION__);

    		// Restore command
    		dataReg_m = 0;
    		trackReg_m = 0xff;
    		curCommand_m = restoreCmd;
    	}
    	else
    	{
    		curCommand_m = seekCmd;
    	}
    	seekTo();
    }
    else
    {
    	// One of the Step commands
	    debugss(ssH37, INFO, "%s - Step\n", __FUNCTION__);
	    curCommand_m = stepCmd;

	    // check for step in or step out.
    	if ((cmd & 0x40) == 0x40)
    	{
    		if ((cmd & 0x20) == 0x20)
    		{
    			// Step Out
        	    debugss(ssH37, INFO, "%s - Step Out\n", __FUNCTION__);
    			stepDirection_m = dir_out;
    		}
    		else
    		{
    			// Step In
        	    debugss(ssH37, INFO, "%s - Step In\n", __FUNCTION__);
    			stepDirection_m = dir_in;
    		}
    	}
		step();
    }
}

void Z_89_37::processCmdTypeII(BYTE cmd)
{
	multiple_m     = ((cmd & cmdop_MultipleRecord_c) == cmdop_MultipleRecord_c);
	delay_m        = ((cmd & cmdop_Delay_15ms_c) == cmdop_Delay_15ms_c);
	sectorLength_m = ((cmd & cmdop_SectorLength_c) == cmdop_SectorLength_c);
	side_m         = ((cmd & cmdop_CompareSide_c) >> cmdop_CompareSide_Shift_c );

    debugss(ssH37, INFO, "%s - cmd: %d\n", __FUNCTION__, cmd);
	if ((cmd & 0x20) == 0x20)
	{
		// Write Sector
		deleteDAM_m = ((cmd & 0x01) == 0x01);

	    debugss(ssH37, INFO, "%s - Write Sector - \n", __FUNCTION__);
	    curCommand_m = writeSectorCmd;

	}
	else
	{
		// Read Sector
	    debugss(ssH37, INFO, "%s - Read Sector: %d - multi: %d delay: %d sector: %d side: %d\n",
	            __FUNCTION__, sectorReg_m, multiple_m, delay_m, sectorLength_m, side_m);
	    curCommand_m = readSectorCmd;

        lostDataStatus_m = false;
        dataReady_m = false;
	    sectorPos_m = -1000;
	}
}

void Z_89_37::processCmdTypeIII(BYTE cmd)
{
    delay_m = ((cmd & cmdop_Delay_15ms_c) == cmdop_Delay_15ms_c);
    side_m = ((cmd & cmdop_CompareSide_c) >> cmdop_CompareSide_Shift_c );

    debugss(ssH37, INFO, "%s - cmd: %d\n", __FUNCTION__, cmd);
    if ((cmd & 0x20) == 0x00)
    {
        // Read Address
        debugss(ssH37, INFO, "%s - Read Address\n", __FUNCTION__);
        curCommand_m = readAddressCmd;

    }
    else if ((cmd & 0x10) == 0x10)
    {
		// write Track
	    debugss(ssH37, INFO, "%s - Write Track: %d\n", __FUNCTION__, trackReg_m);
	    curCommand_m = writeTrackCmd;

	}
	else
	{
		// read Track
	    debugss(ssH37, INFO, "%s - Read Track: %d\n", __FUNCTION__, trackReg_m);
	    curCommand_m = readTrackCmd;

	}
}

void Z_89_37::processCmdTypeIV(BYTE cmd)
{
    debugss(ssH37, INFO, "%s - cmd: 0x%02x\n", __FUNCTION__, cmd);

    curCommand_m = forceInterruptCmd;
    // check to see if previous command is still running
    if (statusReg_m & stat_Busy_c)
    {
        debugss(ssH37, INFO, "%s - Aborting Command\n", __FUNCTION__);
        // still running, abort command and reset busy
        abortCmd();
        statusReg_m &= ~stat_Busy_c;
    }
    else
    {
        bool hole = false, trackZero = false, writeProtect = false;

        // no Command running, update status.
        statusReg_m = 0;

        if ((curDiskDrive_m < numDisks_c) && (drives_m[curDiskDrive_m]))
        {
            drives_m[curDiskDrive_m]->getControlInfo(curPos_m, hole, trackZero, writeProtect);
        }

        if (hole) // check indexPulse
        {
            statusReg_m |= stat_IndexPulse_c;
        }
        /// \todo this needs to move the floppy disk drive object
        if (trackZero)
        {
            statusReg_m |= stat_TrackZero_c;
        }
        if (0) /// check crcError - Don't think it could happen here.
        {
            statusReg_m |= stat_CRCError_c;
        }
        if (0) // check seekError - Don't think it could happen here.
        {
            statusReg_m |= stat_SeekError_c;
        }
        // check head loaded - check with floppy drive
        if ((curDiskDrive_m < numDisks_c) &&
            (drives_m[curDiskDrive_m]) &&
            (drives_m[curDiskDrive_m]->getHeadLoadStatus()))
        {
            statusReg_m |= stat_HeadLoaded_c;
        }
        // check writeProtected...
        if (writeProtect)
        {
            statusReg_m |= stat_WriteProtect_c;
        }
        if (0) // check Not Ready..
        {
            /// \todo figure out what this comes from.
            statusReg_m |= stat_NotReady_c;
        }

        debugss(ssH37, INFO, "%s - updating statusReg: %d\n", __FUNCTION__, statusReg_m);
    }
    if ((cmd & 0x0f) != 0x00)
    {
        // at least one bit is set.
        if ((cmd & cmdop_NotReadyToReady_c) == cmdop_NotReadyToReady_c)
        {
            debugss(ssH37, INFO, "%s - Not Ready to Ready\n", __FUNCTION__);

        }
        if ((cmd & cmdop_ReadyToNotReady_c) == cmdop_ReadyToNotReady_c)
        {
            debugss(ssH37, INFO, "%s - Ready to Not Ready\n", __FUNCTION__);

        }
        if ((cmd & cmdop_IndexPulse_c) == cmdop_IndexPulse_c)
        {
            debugss(ssH37, INFO, "%s - Index Pulse\n", __FUNCTION__);

        }
        if ((cmd & cmdop_ImmediateInterrupt_c) == cmdop_ImmediateInterrupt_c)
        {
            debugss(ssH37, INFO, "%s - Immediate Interrupt\n", __FUNCTION__);

        }
    }
    else
    {
        debugss(ssH37, INFO, "%s - No Interrupt/ Clear Busy\n", __FUNCTION__);
        statusReg_m &= ~(stat_Busy_c);
        curCommand_m = noneCmd;
    }
}

bool Z_89_37::connectDrive(BYTE unitNum, DiskDrive *drive)
{
    bool retVal = false;

    debugss(ssH37, INFO, "%s: unit (%d), drive (%p)\n", __FUNCTION__, unitNum, drive);
    if (unitNum < numDisks_c)
    {
        if (drives_m[unitNum] == 0)
        {
            drives_m[unitNum] = drive;
            retVal = true;
        }
        else
        {
            debugss(ssH37, ERROR, "%s: drive already connect\n", __FUNCTION__);
        }
    }
    else
    {
        debugss(ssH37, ERROR, "%s: Invalid unit number (%d)\n", __FUNCTION__, unitNum);
    }

    return(retVal);
}

void Z_89_37::seekTo()
{
    debugss(ssH37, INFO, "%s - %d\n", __FUNCTION__, dataReg_m);

	if (dataReg_m)
	{
		// non zero
	    debugss(ssH37, INFO, "%s - %d\n", __FUNCTION__, dataReg_m);

	}
	else
	{
		// track is zero, watch for the track 0 status from the drive.
	    statusReg_m |= stat_TrackZero_c;

	}
	statusReg_m &= ~stat_Busy_c;

	trackReg_m = dataReg_m;
	// should really delay until raising the intr.
	/// \todo Get notified when the time is up.
	raiseIntrq();

}

void Z_89_37::step(void)
{
	// direction is set in stepDirection_m
    debugss(ssH37, INFO, "%s\n", __FUNCTION__);

    if (stepDirection_m == dir_out)
    {
        debugss(ssH37, INFO, "%s - step out\n", __FUNCTION__);
        if (trackReg_m)
        {
            trackReg_m--;
        }
    }
    else if (stepDirection_m == dir_in)
    {
        debugss(ssH37, INFO, "%s - step in\n", __FUNCTION__);
        if (trackReg_m)
        trackReg_m++;
    }
}

bool Z_89_37::removeDrive(BYTE unitNum)
{

    return(false);
}

void Z_89_37::abortCmd()
{
    debugss(ssH37, INFO, "%s\n", __FUNCTION__);

    curCommand_m = noneCmd;
}

void Z_89_37::raiseIntrq()
{
	// check if IRQs are allowed.
    debugss(ssH37, INFO, "%s\n", __FUNCTION__);
	if (intrqAllowed_m)
	{
		h89.raiseINT(z_89_37_Intr_c);
		return;
	}
}

void Z_89_37::raiseDrq()
{
    debugss(ssH37, INFO, "%s\n", __FUNCTION__);
	// check if DRQ is allowed.
	if (drqAllowed_m)
	{
//		h89.raiseINT(z_89_37_Intr_c);
        h89.continueCPU();
		return;
	}
    debugss(ssH37, INFO, "%s - not allowed.\n", __FUNCTION__);

}

void Z_89_37::lowerIntrq()
{
    debugss(ssH37, INFO, "%s\n", __FUNCTION__);
	if (!intrqAllowed_m)
	{
		h89.lowerINT(z_89_37_Intr_c);
		return;
	}
}

void Z_89_37::lowerDrq()
{
    debugss(ssH37, INFO, "%s\n", __FUNCTION__);
	if (!drqAllowed_m)
	{
//        h89.lowerINT(z_89_37_Intr_c);
		return;
	}

}

void Z_89_37::loadHead()
{
    debugss(ssH37, INFO, "%s\n", __FUNCTION__);
	if ((curDiskDrive_m < numDisks_c)  && (drives_m[curDiskDrive_m]))
	{
	    drives_m[curDiskDrive_m]->loadHead();
	}
}

void Z_89_37::unloadHead()
{
    debugss(ssH37, INFO, "%s\n", __FUNCTION__);
	if ((curDiskDrive_m < numDisks_c)  && (drives_m[curDiskDrive_m]))
	{
        drives_m[curDiskDrive_m]->unLoadHead();
	}
}


void Z_89_37::notification(unsigned int cycleCount)
{
    unsigned long charPos = 0;

    if (motorOn_m)
    {
        cycleCount_m += cycleCount;
        cycleCount_m %= (bytesPerTrack_c * clocksPerByte_c);
        charPos = cycleCount_m / clocksPerByte_c;
        if (charPos == curPos_m)
        {
            // Position hasn't changed just return
            return;
        }
        debugss(ssH37, ALL, "New character Pos - old: %ld, new: %ld\n", curPos_m, charPos);
        curPos_m = charPos;
    }
    else
    {
        // Drive motor is not turned on. Nothing to do.
        /// \todo determine if we need to use clock to determine when these occur.
        // These are needed for drive detection.
#if 0
        if (drives_m[curDiskDrive_m])
        {
            transmitterBufferEmpty_m = true;
            fillCharTransmitted_m = true;
        }
        else
        {
            transmitterBufferEmpty_m = false;
            fillCharTransmitted_m = false;
        }
#endif
        return;
    }

    if (!(curDiskDrive_m < numDisks_c))
    {
        debugss(ssH37, WARNING, "%s: Invalid Drive: %d\n", __FUNCTION__, curDiskDrive_m);
        return;
    }

#if 0
    switch (state_m)
    {
    case idleState:
        debugss(ssH17, INFO, "%s: Idle State\n", __FUNCTION__);
        /// do nothing
        break;

    case seekingSyncState:
        // determine if to the next character
        // check to see if character matches sync, -> set set sync found
        // also load data buffer.
        if (drives_m[curDrive_m])
        {
            data = drives_m[curDrive_m]->readData(curCharPos);
            debugss(ssH17, ALL, "%s: Seeking Sync(disk: %d): %d\n", __FUNCTION__,
                    curDrive_m, data);
        }
        else
        {
            debugss(ssH17, ERROR, "%s: Seeking Sync - No Drive(%d)\n",
                    __FUNCTION__, curDrive_m);
            // should we return here...
        }

        if (data == syncChar_m)
        {
            debugss(ssH17, INFO, "%s: found sync\n",__FUNCTION__);
            receiverOutputRegister_m = data;
            syncCharacterReceived_m = true;
            receiveDataAvail_m = true; /// \todo determine if this should be set.
            state_m = readingState;
        }
        break;

    case readingState:
        // determine if new character is to be moved into receive buffer,
        //   if so, determine if receive buffer is empty.
        //     if not, set ReceiverOverrun_Flag
        //   store new character in receiver buffer
        //
        if (receiveDataAvail_m)
        {
            debugss(ssH17, INFO, "%s: Receiver Overrun\n",__FUNCTION__);
            // last data byte was not read, set overrun
            receiverOverrun_m = true;
        }
        if (drives_m[curDrive_m])
        {
            data = drives_m[curDrive_m]->readData(curCharPos);
        }
        debugss(ssH17, ALL, "%s: Reading - Pos: %ld Data: %d\n",
                __FUNCTION__, curCharPos, data);
        receiverOutputRegister_m = data;
        receiveDataAvail_m = true;
        break;

    case writingState:
        // Determine if transmitter Holding is empty,
        //    if so,
        //         write fill character.
        //         set fill character transmitted flag.
        //    else
        //         transmit from buffer, empty buffer.
        if (transmitterBufferEmpty_m)
        {
            data = fillChar_m;
            fillCharTransmitted_m = true;
            debugss(ssH17, ALL, "%s: fill char sent Pos: %ld\n",
                    __FUNCTION__, curCharPos);
        }
        else
        {
            data = transmitterHoldingRegister_m;
            transmitterBufferEmpty_m = true;
            debugss(ssH17, ALL, "%s: Writing - Pos: %ld Data: %d\n", __FUNCTION__,
                    curCharPos, data);
        }

        if (drives_m[curDrive_m])
        {
            drives_m[curDrive_m]->writeData(curCharPos, data);
        }
        else
        {
            debugss(ssH17, INFO, "%s: No Valid Drive - Pos: %ld\n", __FUNCTION__, curCharPos);
        }
        break;
    }
#endif

	switch (curCommand_m)
	{
	case restoreCmd:

		break;
	case seekCmd:

		break;
	case stepCmd:

		break;
	case readSectorCmd:
        debugss(ssH37, ALL, "%s: sectorPos_m: %d\n", __FUNCTION__,
                sectorPos_m);

        if ((sectorPos_m >= 0) && (sectorPos_m < 256))
        {
            debugss(ssH37, VERBOSE, "%s: Sector(%d) Read[%d]\n", __FUNCTION__,
                    sectorReg_m, sectorPos_m);

            if ((curDiskDrive_m < numDisks_c)  && (drives_m[curDiskDrive_m]))
            {
                if (dataReady_m)
                {
                    debugss(ssH37, WARNING, "%s: Data Lost: %d\n", __FUNCTION__, curDiskDrive_m);

                    lostDataStatus_m = true;
                }

                // Since Sectors are stored 0 to (n-1), and controller uses 1 to n, must change
                // sectorReg_m to be 0 offset
                dataReg_m = drives_m[curDiskDrive_m]->readSectorData(sectorReg_m - 1,
                                                                     sectorPos_m);

                debugss(ssH37, VERBOSE, "%s: Sector(%d) Read[%d] = %d\n", __FUNCTION__,
                        sectorReg_m, sectorPos_m, dataReg_m);

                dataReady_m = true;
                statusReg_m |= stat_DataRequest_c;

                raiseDrq();
            }
            else
            {
                /// \todo generate error
                debugss(ssH37, WARNING, "%s: Not valid drive - %d\n", __FUNCTION__,
                        curDiskDrive_m);

            }
        }
        sectorPos_m++;
        if (sectorPos_m == 256)
        {
            /// \todo check for multiple sectors.
            curCommand_m = noneCmd;
        }


		break;
	case writeSectorCmd:

		break;
	case readAddressCmd:

		break;
	case readTrackCmd:

		break;
	case writeTrackCmd:

		break;
	case forceInterruptCmd:

		break;
	case noneCmd:

		break;
	}

}

