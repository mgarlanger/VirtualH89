///
/// \file h17.cpp
///
/// \brief Implements the hard-sectored disk controller - H-88-1
///
/// \date Apr 18, 2009
/// \author Mark Garlanger
///


#include "h17.h"

#include "H89.h"
#include "logger.h"
#include "WallClock.h"
#include "DiskDrive.h"
#include "HardSectoredDisk.h"


H17::H17(int baseAddr): DiskController(baseAddr, H17_NumPorts_c),
                        GppListener(h17_gppSideSelectBit_c),
                        state_m(idleState),
                        spinCycles_m(0),
                        curCharPos_m(0),
                        motorOn_m(false),
                        writeGate_m(false),
                        direction_m(false),
                        syncCharacterReceived_m(false),
                        receiveDataAvail_m(false),
                        receiverOverrun_m(false),
                        fillCharTransmitted_m(false),
                        transmitterBufferEmpty_m(true),
                        receiverOutputRegister_m(0),
                        transmitterHoldingRegister_m(0),
                        curDrive_m(maxDiskDrive_c),
                        fillChar_m(0),
                        syncChar_m(0xfd)
{
    for (int i = 0; i < maxDiskDrive_c; ++i)
    {
        drives_m[i] = nullptr;
    }

    GppListener::addListener(this);
}

H17::~H17()
{
    for (BYTE i = 0; i < maxDiskDrive_c; ++i)
    {

        if (drives_m[i])
        {
            std::string diskName = "h17_saveDisk";
            diskName += ('0' + i + 1);
            diskName += ".rawdisk";

            drives_m[i]->ejectDisk(diskName.c_str());
        }
    }
}


H17*
H17::install_H17(BYTE                        baseAddr,
                 PropertyUtil::PropertyMapT& props,
                 std::string                 slot)
{
    std::string                   s;
    H17*                          h17 = new H17(baseAddr);
    debugss(ssH17, INFO, "entering\n");

    for (BYTE i = 0; i < maxDiskDrive_c; ++i)
    {

        std::string prop = "h17_drive";
        prop += ('0' + i + 1);
        s     = props[prop];

        if (!s.empty())
        {

            DiskDrive* drive = DiskDrive::getInstance(s);
            if (drive)
            {

                h17->connectDrive(i, drive);
                prop  = "h17_disk";
                prop += ('0' + i + 1);
                s     = props[prop];

                if (!s.empty())
                {
                    HardSectoredDisk* disk = new HardSectoredDisk(s.c_str());

                    drive->insertDisk(disk);
                }

            }
        }
    }

    return h17;
}


void
H17::gppNewValue(BYTE gpo)
{
    selectSide((gpo & h17_gppSideSelectBit_c) ? 1 : 0);
}

BYTE
H17::in(BYTE addr)
{
    BYTE val    = 0;
    BYTE offset = getPortOffset(addr);

    switch (offset)
    {
        case DataPortOffset_c:
            /// \todo determine if checking receiveDataAvail_m should be done.
            val                = receiverOutputRegister_m;
            receiveDataAvail_m = false;
            debugss(ssH17, INFO, " h17.in(Data) - 0x%02x\n", val);
            break;

        case StatusPortOffset_c:
            if (transmitterBufferEmpty_m)
            {
                val |= TransmitterBufferEmpty_Flag;
            }

            if (fillCharTransmitted_m)
            {
                val                  |= FillCharTransmitted_Flag;
                fillCharTransmitted_m = false;
            }

            if (receiverOverrun_m)
            {
                // \todo determine flag should be set to false after the read
                val |= ReceiverOverrun_Flag;
            }

            if (receiveDataAvail_m)
            {
                val |= ReceiveDataAvail_Flag;
            }

            debugss(ssH17, INFO, " h17.in(Status) - 0x%02x\n", val);
            break;

        case SyncPortOffset_c:
            // SyncPort data terminates at the controller.
            val                     = syncChar_m;

            // Based on a comment in the monitor code.. reading the port sets the searching.
            state_m                 = seekingSyncState;
            syncCharacterReceived_m = false;

            debugss(ssH17, INFO, " h17.in(Sync) - 0x%02x\n", val);
            break;

        case ControlPortOffset_c:

            // Majority of ControlPort is related to the Disk Drive, only sync detect is specific
            // to the controller.

            if (curDrive_m < maxDiskDrive_c)
            {
                // get info from the drive - if no drive attached, the auto-detection will
                // correctly detect no drive.
                if (drives_m[curDrive_m])
                {
                    bool hole, trackZero, writeProtect;

                    drives_m[curDrive_m]->getControlInfo(spinCycles_m / CPUCyclesPerByte_c,
                                                         hole, trackZero, writeProtect);

                    if (hole)
                    {
                        val |= H17::ctrlHoleDetect_Flag;
                    }

                    if (trackZero)
                    {
                        val |= H17::ctrlTrackZeroDetect_Flag;
                    }

                    if (writeProtect)
                    {
                        // disk is write protected.
                        val |= H17::ctrlWriteProtect_Flag;
                    }
                }
                else
                {
                    debugss(ssH17, INFO, " h17.in(Control) - No drive [%d]\n", curDrive_m);
                }
            }
            else
            {
                debugss(ssH17, INFO, " h17.in(Control) - Invalid drive [%d]\n", curDrive_m);
            }

            // Get the sync info directly from the controller.
            if (syncCharacterReceived_m)
            {
                val                    |= ctrlSyncDetect_Flag;
                syncCharacterReceived_m = false;
            }

            debugss(ssH17, INFO, " h17.in(Control) - 0x%02x\n", val);
            break;

        default:
            debugss(ssH17, ERROR, "h17.in(0x%02x) - invalid port\n", addr);
            break;
    }

    return (val);
}

void
H17::out(BYTE addr,
         BYTE val)
{
    BYTE offset = getPortOffset(addr);

    switch (offset)
    {
        case DataPortOffset_c:
            debugss(ssH17, INFO, " h17.out(Data) - 0x%02x\n", val);

            if (!(curDrive_m < maxDiskDrive_c))
            {
                debugss(ssH17, WARNING, " h17.out(Data) - No drive selected\n");
            }

            if (!transmitterBufferEmpty_m)
            {
                debugss(ssH17, ERROR, "Overwriting Transmitter Holding Register\n");
            }

            /// No error status to indicate that the THR was overwritten.
            transmitterHoldingRegister_m = val;
            transmitterBufferEmpty_m     = false;
            break;

        case FillPortOffset_c:
            debugss(ssH17, ERROR, " h17.out(Fill) - 0x%02x\n", val);

            fillChar_m = val;
            break;

        case SyncPortOffset_c:
            debugss(ssH17, INFO, " h17.out(Sync) - 0x%02x\n", val);

            syncChar_m = val;
            break;

        case ControlPortOffset_c:
            debugss(ssH17, INFO, " h17.out(Control) -");

            if (val & WriteGate_Ctrl)
            {
                debugss_nts(ssH17, INFO, " WR");
                writeGate_m = true;
                state_m     = writingState;
            }
            else
            {
                writeGate_m = false;
                state_m     = idleState;
            }

            // On a real system, if multiple bits are set, multiple drives would respond to the
            // request. On the virtual system, it will pick the first one to use. Selecting
            // multiple bits is an error. With the right software, these 3 bits, could actually
            // encode up to 7 drives (000 - would be invalid).
            if (val & DriveSelect0_Ctrl)
            {
                debugss_nts(ssH17, INFO, " DS0");

                curDrive_m = ds0;
            }
            else if (val & DriveSelect1_Ctrl)
            {
                debugss_nts(ssH17, INFO, " DS1");

                curDrive_m = ds1;
            }
            else if (val & DriveSelect2_Ctrl)
            {
                debugss_nts(ssH17, INFO, " DS2");

                curDrive_m = ds2;
            }
            else
            {
                debugss_nts(ssH17, INFO, " DS-");

                curDrive_m = maxDiskDrive_c;
            }

            if (val & Direction_Ctrl) // Zero equals out.
            {
                debugss_nts(ssH17, INFO, " Dir-in");
                direction_m = true;
            }
            else
            {
                debugss_nts(ssH17, INFO, " Dir-out");
                direction_m = false;
            }


            if (val & MotorOn_Ctrl)
            {
                debugss_nts(ssH17, INFO, " MtrOn");

                motorOn_m = true;
            }
            else
            {
                debugss_nts(ssH17, INFO, " MtrOff");

                motorOn_m = false;
            }

            /// Need to terminate the debug status print, since the following bits call
            /// other routines, which may also send output to the logger.
            debugss_nts(ssH17, INFO, "\n");

            /// \todo Determine what state the RAM is in on power-up, keep track here and only
            /// notify when there is a change.
            if (val & WriteEnableRAM_Ctrl)
            {
                h89.writeEnableH17RAM();
            }
            else
            {
                h89.writeProtectH17RAM();
            }

            if (val & StepCommand_Ctrl)
            {
                if ((curDrive_m < maxDiskDrive_c) && (drives_m[curDrive_m]))
                {
                    drives_m[curDrive_m]->step(direction_m);
                }
                else
                {
                    // all drives have been unselected...
                    debugss(ssH17, WARNING, " h17.out(Control) - step --No drive selected\n");
                }
            }

            break;

        default:
            debugss(ssH17, ERROR, "h17.out(0x%02x, 0x%02x) - invalid port\n", addr, val);
            break;

    }
}



bool
H17::connectDrive(BYTE       unitNum,
                  DiskDrive* drive)
{
    bool retVal = false;

    debugss(ssH17, INFO, "unit (%d), drive (%p)\n", unitNum, drive);

    if (unitNum < maxDiskDrive_c)
    {
        if (drives_m[unitNum] == 0)
        {
            drives_m[unitNum] = drive;
            retVal            = true;
        }
        else
        {
            debugss(ssH17, ERROR, "drive already connect\n");
        }
    }
    else
    {
        debugss(ssH17, ERROR, "Invalid unit number (%d)\n", unitNum);
    }

    return (retVal);
}

bool
H17::removeDrive(BYTE unitNum)
{
    bool retVal = false;

    debugss(ssH17, INFO, "unit (%d)\n", unitNum);

    if (curDrive_m < maxDiskDrive_c)
    {
        if (drives_m[unitNum] != 0)
        {
            drives_m[unitNum] = 0;
            retVal            = true;
        }
        else
        {
            debugss(ssH17, WARNING, "no unit to remove (%d)\n", unitNum);
        }
    }
    else
    {
        debugss(ssH17, ERROR, "Invalid unit number (%d)\n", unitNum);
    }

    return (retVal);
}

void
H17::selectSide(BYTE side)
{
    for (int i = 0; i < maxDiskDrive_c; i++)
    {
        if (drives_m[i])
        {
            drives_m[i]->selectSide(side);
        }
    }
}

void
H17::notification(unsigned int cycleCount)
{
    unsigned long charPos = 0;
    BYTE          data    = 0;

    if ((curDrive_m >= maxDiskDrive_c) || (!drives_m[curDrive_m]))
    {
        return;
    }

    spinCycles_m = (spinCycles_m + cycleCount) % (BytesPerTrack_c * CPUCyclesPerByte_c);

    charPos      = spinCycles_m / CPUCyclesPerByte_c;

    if (charPos == curCharPos_m)
    {
        // Position hasn't changed just return
        return;
    }

    debugss(ssH17, ALL, "New character Pos - old: %ld, new: %ld\n", curCharPos_m,
            charPos);
    curCharPos_m = charPos;

    // handle the transmit buffer whether or not writing is going on, this is
    // needed to handle drive detection.
    // data variable will be used later if currently writing.
    if (transmitterBufferEmpty_m)
    {
        data                  = fillChar_m;
        fillCharTransmitted_m = true;
    }
    else
    {
        data                     = transmitterHoldingRegister_m;
        transmitterBufferEmpty_m = true;
    }

    // if motor is not on, just bail.
    if (!motorOn_m)
    {
        return;
    }

    switch (state_m)
    {
        case idleState:
            debugss(ssH17, VERBOSE, "Idle State\n");
            /// do nothing
            break;

        case seekingSyncState:

            // check to see if character matches sync, -> set set sync found
            // also load data buffer.
            data = drives_m[curDrive_m]->readData(curCharPos_m);
            debugss(ssH17, ALL, "Seeking Sync(disk: %d): %d\n", curDrive_m, data);

            if (data == syncChar_m)
            {
                debugss(ssH17, INFO, "found sync\n");
                receiverOutputRegister_m = data;
                syncCharacterReceived_m  = true;
                receiveDataAvail_m       = true; /// \todo determine if this should be set.
                state_m                  = readingState;
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
                /// \todo determine why this is coming out when idle - set back to info
                debugss(ssH17, ALL, "Receiver Overrun\n");
                // last data byte was not read, set overrun
                receiverOverrun_m = true;
            }

            data = drives_m[curDrive_m]->readData(curCharPos_m);

            debugss(ssH17, ALL, "Reading - Pos: %ld Data: %d\n", curCharPos_m, data);

            receiverOutputRegister_m = data;
            receiveDataAvail_m       = true;

            break;

        case writingState:

            drives_m[curDrive_m]->writeData(curCharPos_m, data);

            break;
    }

}
