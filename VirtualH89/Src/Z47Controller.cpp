///
/// \name Z47Controller.cpp
///
/// Controller card for the Z-47 external 8" drives. The card is physically located
/// on the Master Remex Drive.
///
/// \date Aug 12, 2013
/// \author Mark Garlanger
///

#include "Z47Controller.h"

#include "logger.h"
#include "ParallelLink.h"

#include <string.h>

Z47Controller::Z47Controller(): curDisk(invalidDisk_c),
    curLinkState(st_Link_Undefined_c),
    curState(st_None_c),
    statePosition_m(0),
    countDown_m(0),
    linkToHost_m(0),
    readyState(0),
    sectorCount(1),
    driveNotReady_m(false),
    diskWriteProtected_m(false),
    deletedData_m(false),
    noRecordFound_m(false),
    crcError_m(false),
    lateData_m(false),
    invalidCommandReceived_m(false),
    badTrackOverflow_m(false),
    dataToTransmit_m(0),
    drive_m(0),
    side_m(0),
    track_m(0),
    sector_m(0),
    diskOffset(0),
    bytesToTransfer(0),
    sectorSize(128)

{
    // TODO Auto-generated constructor stub
    WallClock::instance()->registerUser(this);

}

Z47Controller::~Z47Controller()
{
    WallClock::instance()->unregisterUser(this);
}


void Z47Controller::loadDisk()
{
    debugss(ssH47, VERBOSE, "%s: Entering\n", __FUNCTION__);

    FILE *file;


    if ((file = fopen("8in.dsk", "r")) != NULL)
    {
        int i = 0;

        for (; i < 26 * 77; i++)
        {
            if ((fread(&diskData[i * 128], 128, 1, file)) != 1)
            {
                debugss(ssH47, INFO, "%s: File ended early: %d\n", __FUNCTION__, i);
            }

            else
            {
                debugss(ssH47, ALL, "%s: File was read\n", __FUNCTION__);
                break;
            }

        }

        for (; i < 26 * 77; i++)
        {
            memset(&diskData[i * 128], 0, 128);
        }

        fclose(file);

#if 0

        if ((fread(&diskData[0], 128 * 26 * 77, 1, file)) != 1)
        {
            debugss(ssH47, ERROR, "%s: Unable to read file\n", __FUNCTION__);
        }

        else
        {
            debugss(ssH47, ALL, "%s: File was read\n", __FUNCTION__);
        }

        fclose(file);
#endif
    }

    else
    {
        debugss(ssH47, ERROR, "%s: unable to open file\n", __FUNCTION__);
    }

}

void Z47Controller::reset()
{
    curDisk = invalidDisk_c;
    curState = st_None_c;
    linkToHost_m = 0;
}

const char *Z47Controller::getStateStr(ControllerState state)
{
    switch (curState)
    {
    case st_None_c:
        return ("None");

    case st_Boot_c:
        return ("Boot");

    case st_ReadCntrlStat_c:
        return ("ReadCntrlStat");

    case st_ReadAuxStat_c:
        return ("ReadAuxStat");

    case st_LoadSectorCount_c:
        return ("LoadSectorCount");

    case st_ReadLastAddr_c:
        return ("ReadLastAddr");

    case st_ReadSectors_c:
        return ("ReadSectors");

    case st_WriteSectors_c:
        return ("WriteSectors");

    case st_ReadSectorsBuffered_c:
        return ("ReadSectorsBuffered");

    case st_WriteSectorsBuffered_c:
        return ("WriteSectorsBuffered");

    case st_WriteSectorsAndDelete_c:
        return ("WriteSectorsAndDelete");

    case st_WriteSectorsBufferedAndDelete_c:
        return ("WriteSectorsBufferedAndDelete");

    case st_Copy_c:
        return ("Copy");

    case st_FormatIBM_SD_c:
        return ("FormatIBM_SD");

    case st_Format_SD_c:
        return ("Format_SD");

    case st_FormatIBM_DD_c:
        return ("FormatIBM_DD");

    case st_Format_DD_c:
        return ("Format_DD");

    case st_ReadReadyStatus_c:
        return ("ReadReadyStatus");

    default:
        break;
    }

    return ("Unknown");
}


void Z47Controller::transitionLinkToReady()
{
    curLinkState = st_Link_Ready_c;

    if (linkToHost_m)
    {
        linkToHost_m->setBusy(false);
        linkToHost_m->setDTR(true);
    }

}

void Z47Controller::transitionLinkToAwaitingReceive()
{
    curLinkState = st_Link_AwaitingToReqReceive_c;

    if (linkToHost_m)
    {
        linkToHost_m->setBusy(false);
        linkToHost_m->setDTR(true);
    }

}

void Z47Controller::transitionLinkToTransmit()
{
    curLinkState = st_Link_AwaitingToTransmit_c;

    if (linkToHost_m)
    {
        linkToHost_m->setBusy(false);
        linkToHost_m->setDTR(true);
    }
}


void Z47Controller::processReadControlStatus(void)
{
    debugss(ssH47, ALL, "%s\n", __FUNCTION__);
    statePosition_m++;

    if (statePosition_m == 2)
    {
        commandComplete();
        return;
    }

    if (statePosition_m != 1)
    {
        debugss(ssH47, ERROR, "%s - Unexpected position\n", __FUNCTION__);

    }

    if (linkToHost_m)
    {
        dataToTransmit_m = 0;

        if (driveNotReady_m)
        {
            debugss(ssH47, INFO, "%s - set Drive not Ready\n", __FUNCTION__);
            dataToTransmit_m |= stat_Cntrl_Drive_Not_Ready_c;
            driveNotReady_m = false;
        }

        if (diskWriteProtected_m)
        {
            debugss(ssH47, INFO, "%s - set Disk Write Protected\n", __FUNCTION__);
            dataToTransmit_m |= stat_Cntrl_Write_Protected_c;
            diskWriteProtected_m = false;
        }

        if (deletedData_m)
        {
            debugss(ssH47, INFO, "%s - set Deleted Data\n", __FUNCTION__);
            dataToTransmit_m |= stat_Cntrl_Deleted_Data_c;
            deletedData_m = false;
        }

        if (noRecordFound_m)
        {
            debugss(ssH47, INFO, "%s - set No Record Found\n", __FUNCTION__);
            dataToTransmit_m |= stat_Cntrl_No_Record_Found_c;
            noRecordFound_m = false;
        }

        if (crcError_m)
        {
            debugss(ssH47, INFO, "%s - set CRC Error\n", __FUNCTION__);
            dataToTransmit_m |= stat_Cntrl_CRC_Error_c;
            crcError_m = false;
        }

        if (lateData_m)
        {
            debugss(ssH47, INFO, "%s - set Late Data\n", __FUNCTION__);
            dataToTransmit_m |= stat_Cntrl_Late_Data_c;
            lateData_m = false;
        }

        if (invalidCommandReceived_m)
        {
            debugss(ssH47, INFO, "%s - set Invalid Command Received\n", __FUNCTION__);
            dataToTransmit_m |= stat_Cntrl_Illegal_Command_c;
            invalidCommandReceived_m = false;
        }

        if (badTrackOverflow_m)
        {
            debugss(ssH47, INFO, "%s - set Bad Track Overflow\n", __FUNCTION__);
            dataToTransmit_m |= stat_Cntrl_Bad_Track_Overflow_c;
            badTrackOverflow_m = false;
        }

        debugss(ssH47, INFO, "%s - cmd_ReadCntrlStat_c - 0x%02x\n", __FUNCTION__, dataToTransmit_m);

        curLinkState = st_Link_AwaitingToTransmit_c;
    }

    else
    {
        /// \todo assert or make the top check an assert
        debugss(ssH47, FATAL, "%s - Link not connected\n", __FUNCTION__);
    }

}

void Z47Controller::processReadAuxStatus(BYTE val)
{
    debugss(ssH47, ALL, "%s\n", __FUNCTION__);
    statePosition_m++;

    if (statePosition_m == 3)
    {
        commandComplete();
        return;
    }

    if (statePosition_m == 2)
    {
        // val passed in is drive and side.
        decodeSideDriveSector(val);

        // based on side_m and drive_m set value to return (by asking drive - which asks disk)
        // but for now just say single sided, single density, and sector length byte of 128
        // (which is hopefully 0);

        dataToTransmit_m = stat_Aux_Side1_Available_c;
        curLinkState = st_Link_AwaitingToTransmit_c;
        countDown_m = 10;
        return;

    }

    if (statePosition_m == 1)
    {
        // val invalid.

        curLinkState = st_Link_AwaitingToReqReceive_c;
        countDown_m = 10;
    }

}

void Z47Controller::processLoadSectorCount(BYTE val)
{
    debugss(ssH47, ALL, "%s\n", __FUNCTION__);
    statePosition_m++;

    if (statePosition_m == 3)
    {
        sectorCount |= val;
        debugss(ssH47, ALL, "%s sector Count = %d\n", __FUNCTION__, sectorCount);

        commandComplete();
        return;
    }

    if (statePosition_m == 2)
    {
        // val passed in is MSB of sector count.
        sectorCount = (val << 8);

        curLinkState = st_Link_AwaitingToReqReceive_c;
        countDown_m = 10;
        return;

    }

    if (statePosition_m == 1)
    {
        // val invalid.

        curLinkState = st_Link_AwaitingToReqReceive_c;
        countDown_m = 10;
    }

}

void Z47Controller::processReadSectorsBufffered(BYTE val)
{
    //debugss(ssH47, ALL, "%s\n", __FUNCTION__);
    statePosition_m++;
    debugss(ssH47, ALL, "%s   statePosition: %d\n", __FUNCTION__, statePosition_m);

    if (bytesToTransfer == 0)
    {
        debugss(ssH47, ALL, "%s All Sectors Read\n", __FUNCTION__);

        commandComplete();
        return;
    }

    if (statePosition_m > 3)
    {
        dataToTransmit_m = diskData[diskOffset++];
        bytesToTransfer--;
        curLinkState = st_Link_AwaitingToTransmit_c;
        countDown_m = 10;
        return;
    }

    if (statePosition_m == 3)
    {
        // val passed in is drive and side.
        decodeSideDriveSector(val);

        diskOffset = track_m * (sectorSize * 26) + ((sector_m - 1) * sectorSize);
        bytesToTransfer = sectorCount * sectorSize - 1;
        // reset sector count
        sectorCount = 1;

        dataToTransmit_m = diskData[diskOffset++];
        curLinkState = st_Link_AwaitingToTransmit_c;
        countDown_m = 10;
        return;
    }

    if (statePosition_m == 2)
    {
        decodeTrack(val);

        // based on side_m and drive_m set value to return (by asking drive - which asks disk)
        // but for now just say single sided, single density, and sector length byte of 128
        // (which is hopefully 0);

        curLinkState = st_Link_AwaitingToReqReceive_c;
        countDown_m = 10;
        return;
    }

    if (statePosition_m == 1)
    {
        // val invalid.

        curLinkState = st_Link_AwaitingToReqReceive_c;
        countDown_m = 10;
    }
}


void Z47Controller::processWriteSectorsBufffered(BYTE val)
{
    //debugss(ssH47, ALL, "%s\n", __FUNCTION__);
    statePosition_m++;
    debugss(ssH47, ALL, "%s   statePosition: %d\n", __FUNCTION__, statePosition_m);

    if (bytesToTransfer == 0)
    {
        diskData[diskOffset++] = val;

        debugss(ssH47, ALL, "%s All Sectors written\n", __FUNCTION__);

        commandComplete();
        return;
    }

    if (statePosition_m > 3)
    {
        diskData[diskOffset++] = val;
        bytesToTransfer--;
        curLinkState = st_Link_AwaitingToReqReceive_c;
        countDown_m = 10;
        return;
    }

    if (statePosition_m == 3)
    {
        // val passed in is drive and side.
        decodeSideDriveSector(val);

        diskOffset = track_m * (sectorSize * 26) + ((sector_m - 1) * sectorSize);
        bytesToTransfer = sectorCount * sectorSize - 1;
        // reset sector count
        sectorCount = 1;

        curLinkState = st_Link_AwaitingToReqReceive_c;
        countDown_m = 10;
        return;
    }

    if (statePosition_m == 2)
    {
        decodeTrack(val);

        // based on side_m and drive_m set value to return (by asking drive - which asks disk)
        // but for now just say single sided, single density, and sector length byte of 128
        // (which is hopefully 0);

        curLinkState = st_Link_AwaitingToReqReceive_c;
        countDown_m = 10;
        return;
    }

    if (statePosition_m == 1)
    {
        // val invalid.

        curLinkState = st_Link_AwaitingToReqReceive_c;
        countDown_m = 10;
    }
}



void Z47Controller::processFormatSingleDensity(BYTE val)
{
    debugss(ssH47, ALL, "%s\n", __FUNCTION__);
    statePosition_m++;

    if (statePosition_m == 3)
    {
        commandComplete();
        return;

    }

    if (statePosition_m == 2)
    {
        //
        decodeSideDriveSector(val);

        countDown_m = 10000;
        memset(&diskData[0], 0xe5, 256256);

        curLinkState =  st_Link_AwaitingControllerComplete_c;
        return;
    }

    if (statePosition_m == 1)
    {
        // val invalid.

        curLinkState = st_Link_AwaitingToReqReceive_c;
        countDown_m = 10;
    }

}


void Z47Controller::processFormatIBMDoubleDensity(BYTE val)
{
    debugss(ssH47, ALL, "%s\n", __FUNCTION__);
    statePosition_m++;

    if (statePosition_m == 3)
    {
        commandComplete();
        return;

    }

    if (statePosition_m == 2)
    {
        //
        decodeSideDriveSector(val);

        countDown_m = 10000;
        memset(&diskData[0], 0xe5, 256256);

        curLinkState =  st_Link_AwaitingControllerComplete_c;
        return;
    }

    if (statePosition_m == 1)
    {
        // val invalid.

        curLinkState = st_Link_AwaitingToReqReceive_c;
        countDown_m = 10;
    }


}

void Z47Controller::processFormatDoubleDensity(BYTE val)
{
    debugss(ssH47, ALL, "%s\n", __FUNCTION__);
    statePosition_m++;

    if (statePosition_m == 3)
    {
        commandComplete();
        return;

    }

    if (statePosition_m == 2)
    {
        //
        decodeSideDriveSector(val);

        countDown_m = 10000;
        memset(&diskData[0], 0xe5, 256256);

        curLinkState =  st_Link_AwaitingControllerComplete_c;
        return;
    }

    if (statePosition_m == 1)
    {
        // val invalid.

        curLinkState = st_Link_AwaitingToReqReceive_c;
        countDown_m = 10;
    }


}


void Z47Controller::processReadReadyStatus(void)
{
    debugss(ssH47, ALL, "%s\n", __FUNCTION__);
    statePosition_m++;

    if (statePosition_m == 2)
    {
        commandComplete();
        return;
    }

    if (statePosition_m != 1)
    {
        debugss(ssH47, ERROR, "%s - Unexpected position\n", __FUNCTION__);
    }

    dataToTransmit_m = readyState;
    curLinkState = st_Link_AwaitingToTransmit_c;
    countDown_m = 10;

    // after reading the status, the changed status bits must be reset.
    readyState &= ~(stat_Ready_Drive0ReadyChanged_c |
                    stat_Ready_Drive1ReadyChanged_c |
                    stat_Ready_Drive2ReadyChanged_c |
                    stat_Ready_Drive3ReadyChanged_c);
}


void Z47Controller::processTransmitted()
{
    if (linkToHost_m)
    {
        linkToHost_m->setDTR(false);
    }

    switch (curState)
    {
    case st_Boot_c:
        debugss(ssH47, ERROR, "%s - Unsupported - st_Boot_c\n", __FUNCTION__);

        break;

    case st_ReadCntrlStat_c:
        debugss(ssH47, INFO, "%s - st_ReadCntrlStat_c\n", __FUNCTION__);
        processReadControlStatus();
        break;

    case st_ReadAuxStat_c:
        debugss(ssH47, INFO, "%s - st_ReadAuxStat_c\n", __FUNCTION__);
        processReadAuxStatus();
        break;

    case st_LoadSectorCount_c:
        debugss(ssH47, ERROR, "%s - st_LoadSectorCount_c\n", __FUNCTION__);
        processLoadSectorCount();
        break;

    case st_ReadLastAddr_c:
        debugss(ssH47, ERROR, "%s - Unsupported - st_ReadLastAddr_c\n", __FUNCTION__);
        break;

    case st_ReadSectors_c:
        debugss(ssH47, ERROR, "%s - Unsupported - st_ReadSectors_c\n", __FUNCTION__);
        break;

    case st_WriteSectors_c:
        debugss(ssH47, ERROR, "%s - Unsupported - st_WriteSectors_c\n", __FUNCTION__);
        break;

    case st_ReadSectorsBuffered_c:
        debugss(ssH47, INFO, "%s - st_ReadSectorsBuffered_c\n", __FUNCTION__);
        //bytesToTransfer = sectorSize;
        processReadSectorsBufffered();
        break;

    case st_WriteSectorsBuffered_c:
        debugss(ssH47, INFO, "%s - st_WriteSectorsBuffered_c\n", __FUNCTION__);
        processReadSectorsBufffered();
        break;

    case st_WriteSectorsAndDelete_c:
        debugss(ssH47, ERROR, "%s - Unsupported - st_WriteSectorsAndDelete_c\n", __FUNCTION__);
        break;

    case st_WriteSectorsBufferedAndDelete_c:
        debugss(ssH47, ERROR, "%s - Unsupported - st_WriteSectorsBufferedAndDelete_c\n", __FUNCTION__);
        break;

    case st_Copy_c:
        debugss(ssH47, ERROR, "%s - Unsupported - st_Copy_c\n", __FUNCTION__);
        break;

    case st_FormatIBM_SD_c:
        debugss(ssH47, ERROR, "%s - Unsupported - st_FormatIBM_SD_c\n", __FUNCTION__);
        break;

    case st_Format_SD_c:
        debugss(ssH47, ERROR, "%s - Unsupported - st_Format_SD_c\n", __FUNCTION__);
        break;

    case st_FormatIBM_DD_c:
        debugss(ssH47, ERROR, "%s - Unsupported - st_FormatIBM_DD_c\n", __FUNCTION__);
        break;

    case st_Format_DD_c:
        debugss(ssH47, ERROR, "%s - Unsupported - st_Format_DD_c\n", __FUNCTION__);
        break;

    case st_ReadReadyStatus_c:
        debugss(ssH47, INFO, "%s - st_ReadReadyStatus_c\n", __FUNCTION__);
        processReadReadyStatus();
        break;

    case st_WaitingToComplete_c:
        debugss(ssH47, ERROR, "%s - Unsupported - st_WaitingToComplete_c\n", __FUNCTION__);
        break;
    }
}

void Z47Controller::processCmd(BYTE cmd)
{
    debugss(ssH47, ALL, "=============START of CMD=============\n");

    debugss(ssH47, ALL, "%s - state - %s: cmd - 0x%02x\n", __FUNCTION__,
            getStateStr(curState), cmd);

    countDown_m = coundDown_Default_c;
    statePosition_m = 0;
    curLinkState = st_Link_Ready_c;

    // first lower DTR, and set busy
    if (linkToHost_m)
    {
        linkToHost_m->setDTR(false);
        linkToHost_m->setBusy(true);
    }


    switch (cmd)
    {
    case cmd_Boot_c:
        debugss(ssH47, ERROR, "%s - Unsupported - cmd_Boot_c %d\n", __FUNCTION__, cmd);
        // not sure if this is used by heathkit boot. Likes like they just read
        // the first 10 sectors.
        curState = st_Boot_c;
        break;

    case cmd_ReadCntrlStat_c:
        debugss(ssH47, INFO, "%s - cmd_ReadCntrlStat_c\n", __FUNCTION__);
        curState = st_ReadCntrlStat_c;
        processReadControlStatus();
        break;

    case cmd_ReadAuxStat_c:
        debugss(ssH47, INFO, "%s - Read Aux Status\n", __FUNCTION__);
        curState  = st_ReadAuxStat_c;
        processReadAuxStatus();
        countDown_m = 20;

        break;

    case cmd_LoadSectorCount_c:
        debugss(ssH47, INFO, "%s - cmd_LoadSectorCount_c\n", __FUNCTION__);
        curState = st_LoadSectorCount_c;
        processLoadSectorCount();
        countDown_m = 20;

        break;

    case cmd_ReadLastAddr_c:
        debugss(ssH47, ERROR, "%s - Unsupported - cmd_ReadLastAddr_c %d\n", __FUNCTION__, cmd);
        break;

    case cmd_ReadSectors_c:
        debugss(ssH47, ERROR, "%s - Unsupported - cmd_ReadSectors_c %d\n", __FUNCTION__, cmd);
        break;

    case cmd_WriteSectors_c:
        debugss(ssH47, ERROR, "%s - Unsupported - cmd_WriteSectors_c %d\n", __FUNCTION__, cmd);
        break;

    case cmd_ReadSectorsBuffered_c:
        debugss(ssH47, INFO, "%s - cmd_ReadSectorsBuffered_c %d\n", __FUNCTION__, cmd);
        curState = st_ReadSectorsBuffered_c;
        bytesToTransfer = sectorSize;
        processReadSectorsBufffered();
        break;

    case cmd_WriteSectorsBuffered_c:
        debugss(ssH47, INFO, "%s - cmd_WriteSectorsBuffered_c %d\n", __FUNCTION__, cmd);
        curState = st_WriteSectorsBuffered_c;
        bytesToTransfer = sectorSize;
        processReadSectorsBufffered();
        break;

    case cmd_WriteSectorsAndDelete_c:
        debugss(ssH47, ERROR, "%s - Unsupported - cmd_WriteSectorsAndDelete_c %d\n", __FUNCTION__, cmd);
        break;

    case cmd_WriteSectorsBufferedAndDelete_c:
        debugss(ssH47, ERROR, "%s - Unsupported - cmd_WriteSectorsBufferedAndDelete_c %d\n",
                __FUNCTION__, cmd);
        break;

    case cmd_Copy_c:
        debugss(ssH47, ERROR, "%s - Unsupported - cmd_Copy_c %d\n", __FUNCTION__, cmd);
        break;

    case cmd_FormatIBM_SD_c:
        debugss(ssH47, ERROR, "%s - Unsupported - cmd_FormatIBM_SD_c %d\n", __FUNCTION__, cmd);
        break;

    case cmd_Format_SD_c:
        debugss(ssH47, INFO, "%s - cmd_Format_SD_c %d\n", __FUNCTION__, cmd);
        curState = st_Format_SD_c;
        processFormatSingleDensity();
        break;

    case cmd_FormatIBM_DD_c:
        debugss(ssH47, INFO, "%s - cmd_FormatIBM_DD_c %d\n", __FUNCTION__, cmd);
        curState = st_FormatIBM_DD_c;
        processFormatIBMDoubleDensity();
        break;

    case cmd_Format_DD_c:
        debugss(ssH47, ERROR, "%s - Unsupported - cmd_Format_DD_c %d\n", __FUNCTION__, cmd);
        curState = st_Format_DD_c;
        processFormatDoubleDensity();
        break;

    case cmd_ReadReadyStatus_c:
        debugss(ssH47, INFO, "%s - Read Ready Status\n", __FUNCTION__);
        curState = st_ReadReadyStatus_c;
        processReadReadyStatus();
        break;

    case cmd_SpecialFunction1_c:
    case cmd_SpecialFunction2_c:
    case cmd_SpecialFunction3_c:
    case cmd_SpecialFunction4_c:
    case cmd_SpecialFunction5_c:
        debugss(ssH47, ERROR, "%s - Unsupported - cmd_SpecialFunctionx_c %d\n", __FUNCTION__, cmd);

        break;

    case cmd_SetDriveCharacteristics_c:
        debugss(ssH47, ERROR, "%s - Unsupported - cmd_SetDriveCharacteristics_c %d\n",
                __FUNCTION__, cmd);
        break;

    case cmd_SeekToTrack_c:
        debugss(ssH47, ERROR, "%s - Unsupported - cmd_SeekToTrack_c %d\n", __FUNCTION__, cmd);
        break;

    case cmd_DiskStatus_c:
        debugss(ssH47, ERROR, "%s - Unsupported - cmd_DiskStatus_c %d\n", __FUNCTION__, cmd);
        break;

    case cmd_ReadLogical_c:
        debugss(ssH47, ERROR, "%s - Unsupported - cmd_ReadLogical_c %d\n", __FUNCTION__, cmd);
        break;

    case cmd_WriteLogical_c:
        debugss(ssH47, ERROR, "%s - Unsupported - cmd_WriteLogical_c %d\n", __FUNCTION__, cmd);
        break;

    case cmd_ReadBufferedLogical_c:
        debugss(ssH47, ERROR, "%s - Unsupported - cmd_ReadBufferedLogical_c %d\n", __FUNCTION__, cmd);
        break;

    case cmd_WriteBufferedLogical_c:
        debugss(ssH47, ERROR, "%s - Unsupported - cmd_WriteBufferedLogical_c %d\n", __FUNCTION__, cmd);
        break;

    case cmd_WriteDeletedDataLogical:
        debugss(ssH47, ERROR, "%s - Unsupported - cmd_WriteDeletedDataLogical %d\n", __FUNCTION__, cmd);
        break;

    case cmd_WriteBufferedDeletedDataLogical_c:
        debugss(ssH47, ERROR, "%s - Unsupported - cmd_WriteBufferedDeletedDataLogical_c %d\n",
                __FUNCTION__,  cmd);
        break;

    default:
        debugss(ssH47, ERROR, "%s - Unknown command - default %d\n", __FUNCTION__, cmd);
        invalidCommandReceived_m = true;

        if (linkToHost_m)
        {
            linkToHost_m->setError(true);
        }

        commandComplete();
        break;
    }

    switch (curLinkState)
    {
    case st_Link_Undefined_c:
        debugss(ssH47, ERROR, "%s - Unexpected Undefined link state\n", __FUNCTION__);
        break;

    case st_Link_Ready_c:
        debugss(ssH47, INFO, "%s - link ready state - do nothing\n", __FUNCTION__);
        break;

    case st_Link_AwaitingToReqReceive_c:
        debugss(ssH47, INFO, "%s - link awaiting receive state\n", __FUNCTION__);
        break;

    case st_Link_AwaitingToTransmit_c:
        debugss(ssH47, INFO, "%s - link transmit state\n", __FUNCTION__);
        break;
    }

}



bool Z47Controller::connectDrive(BYTE unitNum, DiskDrive *drive)
{
    debugss(ssH47, ALL, "%s - %d - %p\n", __FUNCTION__, unitNum, drive);

    if (unitNum < numDrives_c)
    {
        if (drives_m[unitNum] == 0)
        {
            drives_m[unitNum] = drive;
        }

        else
        {
            debugss(ssH47, ERROR, "%s - drive conflict - %d.\n", __FUNCTION__, unitNum);
            return false;
        }
    }

    else
    {
        debugss(ssH47, ERROR, "%s - invalid drive - %d\n", __FUNCTION__, unitNum);
        return false;
    }

    debugss(ssH47, ALL, "%s\n", __FUNCTION__);

    return true;
}

bool Z47Controller::removeDrive(BYTE unitNum)
{
    debugss(ssH47, ALL, "%s\n", __FUNCTION__);

    if (unitNum < numDrives_c)
    {
        if (drives_m[unitNum] == 0)
        {
            debugss(ssH47, WARNING, "%s - no drive to remove - %d.\n", __FUNCTION__, unitNum);
        }

        drives_m[unitNum] = 0;
        debugss(ssH47, ERROR, "%s - disk already in use - %d.\n", __FUNCTION__, unitNum);
    }

    else
    {
        debugss(ssH47, ERROR, "%s - invalid drive - %d\n", __FUNCTION__, unitNum);
    }

    return true;
}

void Z47Controller::notification(unsigned int cycleCount)
{
    switch (curLinkState)
    {
    case st_Link_Undefined_c:
        //debugss(ssH47, ERROR, "%s - Unexpected Undefined link state\n", __FUNCTION__);
        return;

    case st_Link_Ready_c:
        //debugss(ssH47, INFO, "%s - link ready state - do nothing\n", __FUNCTION__);
        return;

    case st_Link_AwaitingToReqReceive_c:
        debugss(ssH47, INFO, "%s - link awaiting to request receive state\n", __FUNCTION__);
        break;

    case st_Link_AwaitingToReceive_c:
        //debugss(ssH47, INFO, "%s - link to receive - do nothing\n", __FUNCTION__);
        // get data when dtak is asserted.
        return;

    case st_Link_AwaitingToTransmit_c:
        debugss(ssH47, INFO, "%s - link transmit state\n", __FUNCTION__);
        break;

    case st_Link_AwaitingTransmitComplete_c:
        return;

    case st_Link_HoldingDTR_c:
        processTransmitted();
        return;

    case st_Link_AwaitingReadyState_c:
        break;

    case st_Link_AwaitingControllerComplete_c:
        break;
    }

    if (countDown_m > cycleCount)
    {
        countDown_m -= cycleCount;
        return;
    }

    countDown_m = 0;

    switch (curLinkState)
    {
    case st_Link_AwaitingToReqReceive_c:
        debugss(ssH47, INFO, "%s - link awaiting receive state\n", __FUNCTION__);

        // just set the flags and wait for the host to send;
        if (linkToHost_m)
        {
            linkToHost_m->setDTR(true);
        }

        curLinkState = st_Link_AwaitingToReceive_c;
        return;

    case st_Link_AwaitingToTransmit_c:
        debugss(ssH47, INFO, "%s - link transmit state\n", __FUNCTION__);

        if (linkToHost_m)
        {
            linkToHost_m->sendHostData(dataToTransmit_m);
        }

        curLinkState = st_Link_AwaitingTransmitComplete_c;
        return;

    case st_Link_AwaitingReadyState_c:
        debugss(ssH47, INFO, "%s - link awaiting ready state\n", __FUNCTION__);

        // just set the flags and wait for the host to send;
        if (linkToHost_m)
        {
            linkToHost_m->setDTR(true);
        }

        curLinkState = st_Link_AwaitingToReceive_c;
        return;
    }

    if (curState == st_None_c)
    {
        debugss(ssH47, ERROR, "%s - Unexpected st_None_c\n", __FUNCTION__);

        return;
    }

    switch (curState)
    {
    case st_Boot_c:
        debugss(ssH47, ERROR, "%s - Unsupported - st_Boot_c\n", __FUNCTION__);

        break;

    case st_ReadCntrlStat_c:
        debugss(ssH47, INFO, "%s - st_ReadCntrlStat_c\n", __FUNCTION__);
        processReadControlStatus();
        break;

    case st_ReadAuxStat_c:
        debugss(ssH47, INFO, "%s - st_ReadAuxStat_c\n", __FUNCTION__);
        processReadReadyStatus();
        break;

    case st_LoadSectorCount_c:
        debugss(ssH47, ERROR, "%s - st_LoadSectorCount_c\n", __FUNCTION__);
        processLoadSectorCount();
        break;

    case st_ReadLastAddr_c:
        debugss(ssH47, ERROR, "%s - Unsupported - st_ReadLastAddr_c\n", __FUNCTION__);
        break;

    case st_ReadSectors_c:
        debugss(ssH47, ERROR, "%s - Unsupported - st_ReadSectors_c\n", __FUNCTION__);
        break;

    case st_WriteSectors_c:
        debugss(ssH47, ERROR, "%s - Unsupported - st_WriteSectors_c\n", __FUNCTION__);
        break;

    case st_ReadSectorsBuffered_c:
        debugss(ssH47, ERROR, "%s - Unsupported - st_ReadSectorsBuffered_c\n", __FUNCTION__);
        break;

    case st_WriteSectorsBuffered_c:
        debugss(ssH47, ERROR, "%s - Unsupported - st_WriteSectorsBuffered_c\n", __FUNCTION__);
        break;

    case st_WriteSectorsAndDelete_c:
        debugss(ssH47, ERROR, "%s - Unsupported - st_WriteSectorsAndDelete_c\n", __FUNCTION__);
        break;

    case st_WriteSectorsBufferedAndDelete_c:
        debugss(ssH47, ERROR, "%s - Unsupported - st_WriteSectorsBufferedAndDelete_c\n", __FUNCTION__);
        break;

    case st_Copy_c:
        debugss(ssH47, ERROR, "%s - Unsupported - st_Copy_c\n", __FUNCTION__);
        break;

    case st_FormatIBM_SD_c:
        debugss(ssH47, ERROR, "%s - Unsupported - st_FormatIBM_SD_c\n", __FUNCTION__);
        break;

    case st_Format_SD_c:
        debugss(ssH47, INFO, "%s - st_Format_SD_c\n", __FUNCTION__);
        processFormatSingleDensity();
        break;

    case st_FormatIBM_DD_c:
        debugss(ssH47, INFO, "%s - st_FormatIBM_DD_c\n", __FUNCTION__);
        processFormatIBMDoubleDensity();

        break;

    case st_Format_DD_c:
        debugss(ssH47, ERROR, "%s - Unsupported - st_Format_DD_c\n", __FUNCTION__);
        processFormatDoubleDensity();
        break;

    case st_WaitingToComplete_c:
        debugss(ssH47, ERROR, "%s - Unsupported - st_WaitingToComplete_c\n", __FUNCTION__);
        break;

    }


}

void Z47Controller::connectHostLink(ParallelLink *link)
{
    debugss(ssH47, ALL, "%s\n", __FUNCTION__);
    linkToHost_m = link;

    if (linkToHost_m)
    {
        linkToHost_m->registerDevice(this);
    }

    else
    {
        debugss(ssH47, ERROR, "%s - link invalid\n", __FUNCTION__);
    }
}


void Z47Controller::processData(BYTE val)
{
    debugss(ssH47, ALL, "%s - 0x%02x\n", __FUNCTION__, val);

    switch (curState)
    {
    case st_None_c:
        debugss(ssH47, ALL, "%s - st_None_c\n", __FUNCTION__);
        processCmd(val);
        break;

    case st_Boot_c:
        debugss(ssH47, ERROR, "%s - st_Boot_c\n", __FUNCTION__);
        break;

    case st_ReadCntrlStat_c:
        debugss(ssH47, ERROR, "%s - Invalid state st_ReadCntrlStat_c\n", __FUNCTION__);
        //processReadControlStatus();
        break;

    case st_ReadAuxStat_c:
        debugss(ssH47, INFO, "%s - st_ReadAuxStat_c - 0x%02x\n", __FUNCTION__, val);
        processReadAuxStatus(val);
        break;

    case st_LoadSectorCount_c:
        debugss(ssH47, INFO, "%s - st_LoadSectorCount_c\n", __FUNCTION__);
        processLoadSectorCount(val);
        break;

    case st_ReadLastAddr_c:
        debugss(ssH47, ERROR, "%s - st_ReadLastAddr_c\n", __FUNCTION__);
        break;

    case st_ReadSectors_c:
        debugss(ssH47, ERROR, "%s - st_ReadSectors_c\n", __FUNCTION__);
        break;

    case st_WriteSectors_c:
        debugss(ssH47, ERROR, "%s - st_WriteSectors_c\n", __FUNCTION__);
        break;

    case st_ReadSectorsBuffered_c:
        debugss(ssH47, INFO, "%s - st_ReadSectorsBuffered_c\n", __FUNCTION__);
        processReadSectorsBufffered(val);
        break;

    case st_WriteSectorsBuffered_c:
        debugss(ssH47, INFO, "%s - st_WriteSectorsBuffered_c\n", __FUNCTION__);
        processWriteSectorsBufffered(val);
        break;

    case st_WriteSectorsAndDelete_c:
        debugss(ssH47, ERROR, "%s - st_WriteSectorsAndDelete_c\n", __FUNCTION__);
        break;

    case st_WriteSectorsBufferedAndDelete_c:
        debugss(ssH47, ERROR, "%s - st_WriteSectorsBufferedAndDelete_c\n", __FUNCTION__);
        break;

    case st_Copy_c:
        debugss(ssH47, ERROR, "%s - st_Copy_c\n", __FUNCTION__);
        break;

    case st_FormatIBM_SD_c:
        debugss(ssH47, ERROR, "%s - st_FormatIBM_SD_c\n", __FUNCTION__);
        break;

    case st_Format_SD_c:
        debugss(ssH47, INFO, "%s - st_Format_SD_c\n", __FUNCTION__);
        processFormatSingleDensity(val);
        break;

    case st_FormatIBM_DD_c:
        debugss(ssH47, INFO, "%s - st_FormatIBM_DD_c\n", __FUNCTION__);
        processFormatIBMDoubleDensity(val);

        break;

    case st_Format_DD_c:
        debugss(ssH47, ERROR, "%s - st_Format_DD_c\n", __FUNCTION__);
        processFormatDoubleDensity(val);
        break;

    default:
        debugss(ssH47, ERROR, "%s - Unknown state\n", __FUNCTION__);

        break;
    }
}

void Z47Controller::raiseSignal(SignalType sigType)
{
    debugss(ssH47, ALL, "%s\n", __FUNCTION__);

    switch (sigType)
    {
    // Master Reset (Host to Disk System)
    case st_MasterReset:
        // Handle a reset
        debugss(ssH47, INFO, "%s: Master Reset received.\n", __FUNCTION__);
        // Set reset flag.
        readyState = stat_Ready_Drive0ReadyChanged_c |
                     stat_Ready_Drive1ReadyChanged_c;

        curLinkState = st_Link_AwaitingToReceive_c;

        if (linkToHost_m)
        {
            linkToHost_m->setBusy(false);
            linkToHost_m->setDTR(true);
        }

        // clear the errors
        driveNotReady_m          = false;
        diskWriteProtected_m     = false;
        deletedData_m            = false;
        noRecordFound_m          = false;
        crcError_m               = false;
        lateData_m               = false;
        invalidCommandReceived_m = false;
        badTrackOverflow_m       = false;


        break;

    // Data Acknowledge (Host to Disk System)
    case st_DTAK:
        // Handle data acknowledge.
        debugss(ssH47, INFO, "%s: DTAK received.\n", __FUNCTION__);

        if (curLinkState == st_Link_AwaitingTransmitComplete_c)
        {
            curLinkState = st_Link_HoldingDTR_c;
            countDown_m = 1;
        }

        else if (curLinkState == st_Link_AwaitingToReceive_c)
        {
            BYTE val = 0;

            if (linkToHost_m)
            {
                linkToHost_m->readDataBusByDrive(val);
            }

            processData(val);
        }

        break;

    // Data Ready (Disk System to Host)
    case st_DTR:
        debugss(ssH47, ERROR, "%s: Invalid DTR received.\n", __FUNCTION__);
        break;

    // Disk Drive Out (Disk System to Host)
    case st_DDOUT:
        debugss(ssH47, ERROR, "%s: Invalid DDOUT received.\n", __FUNCTION__);
        break;

    // Busy (Disk System to Host)
    case st_Busy:
        // Not valid
        debugss(ssH47, ERROR, "%s: Invalid Busy received.\n", __FUNCTION__);
        break;

    // Error (Disk System to Host)
    case st_Error:
        debugss(ssH47, ERROR, "%s: Invalid error received.\n", __FUNCTION__);
        break;

    default:
        debugss(ssH47, ERROR, "%s: Invalid sigType received.\n", __FUNCTION__);
        break;
    }
}

void Z47Controller::lowerSignal(SignalType sigType)
{
    debugss(ssH47, ALL, "%s\n", __FUNCTION__);

    switch (sigType)
    {
    // Master Reset (Host to Disk System)
    case st_MasterReset:
        // Handle a reset
        debugss(ssH47, INFO, "%s: Master Reset received.\n", __FUNCTION__);

        break;

    // Data Acknowledge (Host to Disk System)
    case st_DTAK:
        // Handle data acknowledge.
        // Computer acknowledged the transfer to it, or has valid data
        // available for the disk system to receive.
        debugss(ssH47, INFO, "%s: DTAK received.\n", __FUNCTION__);

        break;

    // Data Ready (Disk System to Host)
    case st_DTR:
        // This signal only goes to the host
        debugss(ssH47, ERROR, "%s: Invalid DTR received.\n", __FUNCTION__);
        break;

    // Disk Drive Out (Disk System to Host)
    case st_DDOUT:
        // This signal only goes to the host
        debugss(ssH47, ERROR, "%s: Invalid DDOUT received.\n", __FUNCTION__);
        break;

    // Busy (Disk System to Host)
    case st_Busy:
        // This signal only goes to the host
        debugss(ssH47, ERROR, "%s: Invalid Busy received.\n", __FUNCTION__);
        break;

    // Error (Disk System to Host)
    case st_Error:
        // This signal only goes to the host
        debugss(ssH47, ERROR, "%s: Invalid error received.\n", __FUNCTION__);
        break;

    default:
        debugss(ssH47, ERROR, "%s: Invalid sigType received.\n", __FUNCTION__);
        break;
    }
}

void Z47Controller::pulseSignal(SignalType sigType)
{
    debugss(ssH47, ALL, "%s\n", __FUNCTION__);

    raiseSignal(sigType);
    lowerSignal(sigType);
}

void Z47Controller::commandComplete(void)
{
    debugss(ssH47, ALL, "%s - state: %s\n", __FUNCTION__, getStateStr(curState));

    curState  = st_None_c;
    curLinkState = st_Link_AwaitingReadyState_c;
    countDown_m = 120;

    if (linkToHost_m)
    {
        linkToHost_m->setBusy(false);
        //linkToHost_m->setDTR(true);
    }

    debugss(ssH47, ALL, "=============END of CMD============\n");

}

void Z47Controller::decodeSideDriveSector(BYTE val)
{
    side_m = (val & sideMask_c) >> sideShift_c;

    drive_m = (val & driveMask_c) >> driveShift_c;

    sector_m = (val & sectorMask_c) >> sectorShift_c;

    debugss(ssH47, ALL, "%s side: %d  drive: %d  sector: %d\n", __FUNCTION__,
            side_m, drive_m, sector_m);
}

void Z47Controller::decodeTrack(BYTE val)
{
    track_m = (val & trackMask_c) >> trackShift_c;
    debugss(ssH47, ALL, "%s track: %d\n", __FUNCTION__, track_m);
}
