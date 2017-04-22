///
/// \name Z47Controller.h
///
///
/// \date Aug 12, 2013
/// \author Mark Garlanger
///

#ifndef Z47CONTROLLER_H_
#define Z47CONTROLLER_H_

#include "ClockUser.h"
#include "ParallelPortConnection.h"


class DiskDrive;
class ParallelLink;

class Z47Controller: virtual public ClockUser, virtual public ParallelPortConnection

{
  public:
    Z47Controller();
    virtual ~Z47Controller() override;

    bool removeDrive(BYTE unitNum);
    bool connectDrive(BYTE       unitNum,
                      DiskDrive* drive);

    virtual void notification(unsigned int cycleCount) override;

    void connectHostLink(ParallelLink* link);


    virtual void raiseSignal(SignalType sigType) override;
    virtual void lowerSignal(SignalType sigType) override;
    virtual void pulseSignal(SignalType sigType) override;

    void loadDisk(void);

  private:

    ///   Format commands
    ///        CP/M
    ///      SD -  Single Density,    26 sectors,  128 bytes
    ///      DD - IBM Double Density, 26 sectors,  256 bytes
    ///      ED - IBM Double Density, 8  sectors, 1024 bytes

    ///
    /// Commands
    ///

    /// Bootstrap (BOOT) Command
    ///
    /// Transfers Track 00 Sectors 1 and 2 of unit 0, side 0 to host.
    ///
    /// Byte   Direction    Value
    ///  0      to H47       0x00
    ///
    static const BYTE cmd_Boot_c = 0x00;

    /// Read Controller Status (RSTS) Command
    ///
    /// Transfer status to host.
    ///
    /// Byte   Direction    Value
    ///  0      to H47       0x01
    ///  1      to Host     <Status>
    ///
    static const BYTE cmd_ReadCntrlStat_c             = 0x01;

    /// Bad Track Overflow - (after initialize) More than 2 bad tracks found on media.
    static const BYTE stat_Cntrl_Bad_Track_Overflow_c = 0x01;

    /// Illegal Command - CMD code specified is not in defined set.
    static const BYTE stat_Cntrl_Illegal_Command_c    = 0x02;

    /// Late Data - Host did not respond in time to DTR*. Data was lost
    static const BYTE stat_Cntrl_Late_Data_c          = 0x04;

    /// CRC Error - CRC computation error on ID or data field
    static const BYTE stat_Cntrl_CRC_Error_c          = 0x08;

    /// No Record Found - Track or Sector not found in 2 revolutions.
    static const BYTE stat_Cntrl_No_Record_Found_c    = 0x10;

    /// Deleted Data - Data field with deleted data mark encountered during last read.
    static const BYTE stat_Cntrl_Deleted_Data_c       = 0x20;

    /// Write Protected - Attempt made to write on a protected media.
    static const BYTE stat_Cntrl_Write_Protected_c    = 0x40;

    /// Drive Not Ready - Operation was attempted on a drive that was not ready, or ready
    /// was lost during an operation
    static const BYTE stat_Cntrl_Drive_Not_Ready_c = 0x80;


    /// Read Auxiliary Status Command
    ///
    /// Transfer Auxiliary Status to host.
    ///
    /// Byte   Direction    Value
    ///  0      to H47       0x02
    ///  1      to H47       <side/drive>
    ///  2      to Host      <Auxilliary Status>
    ///
    static const BYTE cmd_ReadAuxStat_c = 0x02;

    /// Return data
    /// Sector Length bits from I.D. field (TRK 1 - 76)
    static const BYTE stat_Aux_SectorLengthId_Trk01_Mask_c = 0x03;

    /// Sector Length bits from I.D. field (TRK 0)
    static const BYTE stat_Aux_SectorLengthId_Trk00_Mask_c = 0x0C;

    /// Side 1 Available
    static const BYTE stat_Aux_Side1_Available_c           = 0x10;

    /// Double Density (TRK 1 - 76)
    static const BYTE stat_Aux_DoubleDensity_Trk01_c       = 0x20;

    /// Double Density (TRK 0)
    static const BYTE stat_Aux_DoubleDensity_Trk00_c       = 0x40;


    /// Load Sector Count Command (LDSC)
    ///
    /// Load 2 byte sector count for next operation.
    ///
    /// Count = 1 to 65535
    ///
    /// Count = 0 is illegal
    ///
    /// Absence of this command, means a count of 1.
    ///
    /// Byte   Direction    Value
    ///  0      to H47       0x03
    ///  1      to H47       <Sector Count MSB>
    ///  2      to H47       <Sector Count LSB>
    ///
    static const BYTE cmd_LoadSectorCount_c = 0x03;


    /// Read Last Address Command (RADR)
    ///
    /// Transfers to host the last track/side/Unit Sector accessed by
    /// disk subsystem.
    ///
    /// Byte   Direction    Value
    ///  0      to H47       0x04
    ///  1      to Host      <Track>
    ///  2      to Host      <Side/Drive/Sector>
    ///
    static const BYTE cmd_ReadLastAddr_c = 0x04;


    /// Read Sectors (RD) Command
    ///
    /// Transfers data directly to host. Number of sectors transferred depends
    /// on previous LDSC. If LDSC is not specified, one sector is read.
    ///
    ///  *  - Number of bytes(sectors) specified by LDSC
    ///
    /// Byte   Direction    Value
    ///  0      to H47       0x05
    ///  1      to H47       <Track>
    ///  2      to H47       <Side/Drive/Sector>
    ///  *      to Host      <Data>
    ///
    static const BYTE cmd_ReadSectors_c = 0x05;


    /// Write Sectors Command
    ///
    /// Transfers data directly, or through buffer, with or without deleted data
    /// mark to subsystem. Number of sectors written depends on previous LDSC. If
    /// LDSC is not specified, one sector is written.
    ///
    /// Byte   Direction    Value
    ///  0      to H47       0x06
    ///  1      to H47       <Track>
    ///  2      to H47       <Side/Drive/Sector>
    ///  3      to H47       <Data>
    ///
    static const BYTE cmd_WriteSectors_c = 0x06;


    /// Read Sectors Buffered (RDB) Command
    ///
    /// Transfers data through buffer to host. Number of sectors transferred depends
    /// on previous LDSC. If LDSC is not specified, one sector is read.
    ///
    ///  *  - Number of bytes(sectors) specified by LDSC
    ///
    /// Byte   Direction    Value
    ///  0      to H47       0x07
    ///  1      to H47       <Track>
    ///  2      to H47       <Side/Drive/Sector>
    ///  *      to Host      <Data>
    ///
    static const BYTE cmd_ReadSectorsBuffered_c = 0x07;


    /// Write Sectors Buffered Command
    ///
    /// Transfers data directly, or through buffer, with or without deleted data
    /// mark to subsystem. Number of sectors written depends on previous LDSC. If
    /// LDSC is not specified, one sector is written.
    ///
    /// Byte   Direction    Value
    ///  0      to H47       0x08
    ///  1      to H47       <Track>
    ///  2      to H47       <Side/Drive/Sector>
    ///  *      to H47       <Data>
    ///
    static const BYTE cmd_WriteSectorsBuffered_c = 0x08;


    /// Write Sectors and Delete Command
    ///
    /// Transfers data directly, or through buffer, with or without deleted data
    /// mark to subsystem. Number of sectors written depends on previous LDSC. If
    /// LDSC is not specified, one sector is written.
    ///
    /// Byte   Direction    Value
    ///  0      to H47       0x09
    ///  1      to H47       <Track>
    ///  2      to H47       <Side/Drive/Sector>
    ///  *      to H47       <Data>
    ///
    static const BYTE cmd_WriteSectorsAndDelete_c = 0x09;


    /// Write Sectors and Delete Buffered Command
    ///
    /// Transfers data directly, or through buffer, with or without deleted data
    /// mark to subsystem. Number of sectors written depends on previous LDSC. If
    /// LDSC is not specified, one sector is written.
    ///
    /// Byte   Direction    Value
    ///  0      to H47       0x0a
    ///  1      to H47       <Track>
    ///  2      to H47       <Side/Drive/Sector>
    ///  *      to H47       <Data>
    ///
    static const BYTE cmd_WriteSectorsBufferedAndDelete_c = 0x0a;


    /// Copy Command
    ///
    /// Copy number of sectors specified by LDSC from source (Track/Side/Drive/Sector)
    /// to destination (Track/Side/Drive/Sector)
    ///
    /// Byte   Direction    Value
    ///  0      to H47       0x0b
    ///  1      to H47       <Source Track>
    ///  2      to H47       <Source Side/Drive/Sector>
    ///  3      to H47       <Destination Track>
    ///  4      to H47       <Destination Side/Drive/Sector>
    ///
    static const BYTE cmd_Copy_c = 0x0b;


    /// Format IBM Single Density Command
    ///
    /// Formats media as follows:
    /// Track 00, Side: 0: 26 sectors single density
    /// Track 00, Side: 1: 26 sectors double density \todo determine if this a manual error
    /// Tracks 01-76, sides 0 and 1: 26, 15, or 8 sectors (as specified), single density
    ///
    /// Byte   Direction    Value
    ///  0      to H47       0x0c
    ///  1      to H47       <Side/Drive/Number Sectors in TK01-TK76>
    ///
    static const BYTE cmd_FormatIBM_SD_c = 0x0c;


    /// Format Single Density Command
    ///
    /// Formats media as follows:
    /// Tracks 00-76, sides 0 and 1: 26, 15, or 8 sectors (as specified), single density
    ///                              128, 256, 512 bytes per sector
    ///                     1 side/77 tracks/26 sectors/128 bytes = 256256
    ///                     2 side/77 tracks/26 sectors/128 bytes = 512512
    ///                     1 side/77 tracks/15 sectors/256 bytes = 295680
    ///                     2 side/77 tracks/15 sectors/256 bytes = 591360
    ///                     1 side/77 tracks/8 sectors/512 bytes  = 315392
    ///                     2 side/77 tracks/8 sectors/512 bytes  = 630784
    ///
    /// Byte   Direction    Value
    ///  0      to H47       0x0d
    ///  1      to H47       <Side/Drive/Number Sectors in TK00-TK76>
    ///
    static const BYTE cmd_Format_SD_c = 0x0d;


    /// Format IBM Double Density Command
    ///
    /// Formats media as follows:
    /// Track 00, Side: 0: 26 sectors single density
    /// Track 00, Side: 1: 26 sectors double density
    /// Tracks 01-76, sides 0 and 1: 26, 15, or 8 sectors (as specified), double density
    ///
    /// Byte   Direction    Value
    ///  0      to H47       0x0e
    ///  1      to H47       <Side/Drive/Number Sectors in TK01-TK76>
    ///
    static const BYTE cmd_FormatIBM_DD_c = 0x0e;


    /// Format Double Density Command
    ///
    /// Formats media as follows:
    /// Tracks 00-76, sides 0 and 1: 26, 15, or 8 sectors (as specified), double density
    ///                             256, 512  1024 bytes per sector
    ///                     1 side/77 tracks/26 sectors/256 bytes =  512512
    ///                     2 side/77 tracks/26 sectors/256 bytes = 1025024
    ///                     1 side/77 tracks/15 sectors/512 bytes =  591360
    ///                     2 side/77 tracks/15 sectors/512 bytes = 1182720
    ///                     1 side/77 tracks/8 sectors/1024 bytes =  630784
    ///                     2 side/77 tracks/8 sectors/1024 bytes = 1261568
    ///
    /// Byte   Direction    Value
    ///  0      to H47       0x0f
    ///  1      to H47       <Side/Drive/Number Sectors in TK00-TK76>
    ///
    static const BYTE cmd_Format_DD_c                 = 0x0f;


    static const BYTE cmd_ReadReadyStatus_c           = 0x10;

    static const BYTE stat_Ready_Drive0NotReady_c     = 0x01;
    static const BYTE stat_Ready_Drive1NotReady_c     = 0x02;
    static const BYTE stat_Ready_Drive2NotReady_c     = 0x04;
    static const BYTE stat_Ready_Drive3NotReady_c     = 0x08;
    static const BYTE stat_Ready_Drive0ReadyChanged_c = 0x10;
    static const BYTE stat_Ready_Drive1ReadyChanged_c = 0x20;
    static const BYTE stat_Ready_Drive2ReadyChanged_c = 0x40;
    static const BYTE stat_Ready_Drive3ReadyChanged_c = 0x80;

    // special debug commands

    /// Special Debug Function 0 Command
    /// Read Ready Status
    /// Note the overlap with Read Ready Status.
    static const BYTE cmd_SpecialFunction0_c = 0x10;

    /// Special Debug Function 1 Command
    static const BYTE cmd_SpecialFunction1_c = 0x11;

    /// Special Debug Function 2 Command
    static const BYTE cmd_SpecialFunction2_c = 0x12;

    /// Special Debug Function 3 Command
    static const BYTE cmd_SpecialFunction3_c = 0x13;

    /// Special Debug Function 4 Command
    static const BYTE cmd_SpecialFunction4_c = 0x14;

    /// Special Debug Function 5 Command
    static const BYTE cmd_SpecialFunction5_c = 0x15;

    // special Heath Functions

    /// Heath's Set Drive Characteristics Command
    static const BYTE cmd_SetDriveCharacteristics_c         = 0x80;

    /// Heath's Seek to Track Command
    static const BYTE cmd_SeekToTrack_c                     = 0x81;

    /// Heath's Disk Status Command
    static const BYTE cmd_DiskStatus_c                      = 0x82;

    /// Heath's Read Logical Command
    static const BYTE cmd_ReadLogical_c                     = 0x83;

    /// Heath's Write Logical Command
    static const BYTE cmd_WriteLogical_c                    = 0x84;

    /// Heath's Read Buffered Logical Command
    static const BYTE cmd_ReadBufferedLogical_c             = 0x85;

    /// Heath's Write Buffered Logical Command
    static const BYTE cmd_WriteBufferedLogical_c            = 0x86;

    /// Heath's Write Deleted Data Logical Command
    static const BYTE cmd_WriteDeletedDataLogical           = 0x87;

    /// Heath's Write Buffered Deleted Data Logical Command
    static const BYTE cmd_WriteBufferedDeletedDataLogical_c = 0x88;

    /// Dipswitch 0
    static const BYTE ds_sw0_c                              = 0x02;

    /// Dipswitch 1
    static const BYTE ds_sw1_c                              = 0x04;

    /// Dipswitch 2
    static const BYTE ds_sw2_c                              = 0x08;

    /// Dipswitch 3
    static const BYTE ds_sw3_c                              = 0x10;

    enum Drives
    {
        ds0           = 0,
        ds1           = 1,
        ds2           = 2,
        ds3           = 3,
        numDrives_c   = 4,
        invalidDisk_c = numDrives_c
    };

    Drives            curDisk;

    static const WORD sectorSize128_c  = 128;
    static const WORD sectorSize256_c  = 256;
    static const WORD sectorSize512_c  = 512;
    static const WORD sectorSize1024_c = 1024;
    static const WORD sectorSizeMax_c  = sectorSize1024_c;

    enum LinkState
    {
        st_Link_Undefined_c,
        st_Link_Ready_c,
        st_Link_AwaitingToReqReceive_c,
        st_Link_AwaitingToReceive_c,
        st_Link_AwaitingToTransmit_c,
        st_Link_AwaitingTransmitComplete_c,
        st_Link_AwaitingReadyState_c,
        st_Link_AwaitingControllerComplete_c,
        st_Link_HoldingDTR_c // - manual says that DTR is held for 80 nSec, but that must be for
        //                        a hardware requirement, since at 2 MHz, 80 nSec is much less than
        //                        one cpu clock. But to avoid calling back on the host as it is calling
        //                        on the drive we will lower it on the next notification.
    };

    LinkState curLinkState;

    void transitionLinkToReady();
    void transitionLinkToAwaitingReceive();
    void transitionLinkToTransmit();

    enum ControllerState
    {
        st_None_c,
        st_Boot_c,
        st_ReadCntrlStat_c,
        st_ReadAuxStat_c,
        st_LoadSectorCount_c,
        st_ReadLastAddr_c,
        st_ReadSectors_c,
        st_WriteSectors_c,
        st_ReadSectorsBuffered_c,
        st_WriteSectorsBuffered_c,
        st_WriteSectorsAndDelete_c,
        st_WriteSectorsBufferedAndDelete_c,
        st_Copy_c,
        st_FormatIBM_SD_c,
        st_Format_SD_c,
        st_FormatIBM_DD_c,
        st_Format_DD_c,
        st_ReadReadyStatus_c,
        st_WaitingToComplete_c,
        st_MaxState_c
    };


    ControllerState            curState;
    unsigned int               statePosition_m;
    unsigned long long         countDown_m;


    static const unsigned long coundDown_Default_c = 200;

    void processCmd(BYTE val);
    const char* getStateStr(ControllerState state);

    void reset();

    DiskDrive*        drives_m[numDrives_c];
    ParallelLink*     linkToHost_m;

    BYTE              readyState;
    WORD              sectorCount;

    bool              driveNotReady_m;
    bool              diskWriteProtected_m;
    bool              deletedData_m;
    bool              noRecordFound_m;
    bool              crcError_m;
    bool              lateData_m;
    bool              invalidCommandReceived_m;
    bool              badTrackOverflow_m;

    BYTE              dataToTransmit_m;

    // Specified drive/side/track from host.
    BYTE              drive_m;
    BYTE              side_m;
    BYTE              track_m;
    BYTE              sector_m;

    void decodeSideDriveSector(BYTE val);
    static const BYTE sideMask_c    = 0x80;
    static const BYTE sideShift_c   = 7;

    static const BYTE driveMask_c   = 0x60;
    static const BYTE driveShift_c  = 5;

    static const BYTE sectorMask_c  = 0x1F;
    static const BYTE sectorShift_c = 0;

    void decodeTrack(BYTE val);

    static const BYTE trackMask_c  = 0xFF;
    static const BYTE trackShift_c = 0;

    void processReadControlStatus(void);
    void processReadReadyStatus(void);
    void processLoadSectorCount(BYTE val = 0);
    void processReadAuxStatus(BYTE val = 0);
    void processReadSectorsBufffered(BYTE val = 0);
    void processWriteSectorsBufffered(BYTE val = 0);
    void processFormatSingleDensity(BYTE val = 0);
    void processFormatDoubleDensity(BYTE val = 0);
    void processFormatIBMDoubleDensity(BYTE val = 0);

    void processTransmitted(void);
    void commandComplete(void);


    void processData(BYTE val);

    BYTE diskData[256256];
    int  diskOffset;
    int  bytesToTransfer;
    WORD sectorSize;
};

#endif // Z47CONTROLLER_H_
