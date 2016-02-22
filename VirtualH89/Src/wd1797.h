/// \file wd1797.h
///
/// Implementation of the WD1797 FDC (floppy disk controller) chip.
///
/// \date Jan 30, 2016
/// \author Douglas Miller, cloned from h37.h by Mark Garlanger
///

#ifndef WD1797_H_
#define WD1797_H_

#include "IODevice.h"
#include "ClockUser.h"

class GenericFloppyDrive;

///
/// \brief Virtual soft-sectored disk controller
///
/// A virtual Magnolia Microsystems soft-sectored disk controller (MMS77316).
/// Note: this is NOT complete or even marginally functional.
///
/// The MMS77316 uses the 1797-02 controller.
///
class WD1797 : public ClockUser
{
  public:
    WD1797(int baseAddr);
    virtual ~WD1797();

    virtual BYTE in(BYTE addr);
    virtual void out(BYTE addr, BYTE val);

    virtual void reset(void);
    void notification(unsigned int cycleCount);

    void eject(char const *file);
    void dump();

  protected:
    virtual GenericFloppyDrive *getCurDrive() = 0;
    virtual int getClockPeriod() = 0;
    BYTE basePort_m;

    static const BYTE WD1797_NumPorts_c       = 4;

    static const BYTE StatusPort_Offset_c       = 0;
    static const BYTE CommandPort_Offset_c      = 0;
    static const BYTE TrackPort_Offset_c        = 1;
    static const BYTE SectorPort_Offset_c       = 2;
    static const BYTE DataPort_Offset_c         = 3;

    BYTE trackReg_m;
    BYTE sectorReg_m;
    BYTE dataReg_m;
    BYTE cmdReg_m;
    BYTE statusReg_m;

    bool dataReady_m;
    bool intrqRaised_m;
    bool drqRaised_m;
    bool headLoaded_m;
    int sectorLength_m;
    bool lastIndexStatus_m;
    int indexCount_m;
    bool stepUpdate_m;
    unsigned long stepSettle_m;

    /// type I parameters.
    BYTE seekSpeed_m;
    bool verifyTrack_m;

    /// type II parameters.
    bool multiple_m;
    bool delay_m;
    BYTE side_m;
    bool deleteDAM_m;
    BYTE addr_m[6];

    enum Direction
    {
        dir_out = -1,
        dir_in  = 1
    };

    enum State
    {
        idleState,
        seekingSyncState,
        readingState,
        writingState
    };
    State state_m;

    enum Command
    {
        restoreCmd,
        seekCmd,
        stepCmd,        /// shared with step in/step out.
        readSectorCmd,
        writeSectorCmd,
        readAddressCmd,
        readTrackCmd,
        writeTrackCmd,
        forceInterruptCmd,
        // pseudo-commands/states used internally
        stepDoneCmd,
        completedCmd,
        writingSectorCmd,
        writingTrackCmd,
        noneCmd
    };
    Command curCommand_m;

    Direction  stepDirection_m;

    unsigned long int curPos_m;

    int  sectorPos_m;

    void waitForData();
    void processCmd(BYTE cmd);
    void processCmdTypeI(BYTE cmd);
    void processCmdTypeII(BYTE cmd);
    void processCmdTypeIII(BYTE cmd);
    void processCmdTypeIV(BYTE cmd);

    void abortCmd();
    unsigned long millisecToTicks(unsigned long ms);

    // Controller may need to override/trap these.
    virtual void raiseIntrq() = 0;
    virtual void raiseDrq() = 0;
    virtual void lowerIntrq() = 0;
    virtual void lowerDrq() = 0;
    virtual void loadHead(bool load);
    virtual bool doubleDensity() = 0;

    ///
    /// Commands sent to CommandPort_c
    ///
    static const BYTE cmd_Mask_c                 = 0xf0;
    static const BYTE cmd_Restore_c              = 0x00;  // 0000 hVrr
    static const BYTE cmd_SeekTrack_c            = 0x10;  // 0001 hVrr
    static const BYTE cmd_StepRepeat_c           = 0x20;  // 001T hVrr
    static const BYTE cmd_StepIn_c               = 0x40;  // 010T hVrr
    static const BYTE cmd_StepOut_c              = 0x60;  // 011T hVrr
    static const BYTE cmd_ReadSector_c           = 0x80;  // 100m SEC0
    static const BYTE cmd_WriteSector_c          = 0xa0;  // 101m SECa
    static const BYTE cmd_ReadAddress_c          = 0xc0;  // 1100 0E00
    static const BYTE cmd_ReadTrack_c            = 0xe0;  // 1110 0E00
    static const BYTE cmd_WriteTrack_c           = 0xf0;  // 1111 0E00
    static const BYTE cmd_ForceInterrupt_c       = 0xd0;  // 1101 IIII (I3/I2/I1/I0)

    ///
    /// rr - Stepping Motor Rate
    /// ===============================================
    /// 00 -  6 mSec
    /// 01 - 12 mSec
    /// 10 - 20 mSec
    /// 11 - 30 mSec
    ///
    static const BYTE maxStepSpeeds_c            = 4;
    static const BYTE speeds[maxStepSpeeds_c];
    static const BYTE cmdop_StepMask_c           = 0x03;   // 0000 0011
    static const BYTE cmdop_Step6ms_c            = 0x00;   // 0000 0000
    static const BYTE cmdop_Step12ms_c           = 0x01;   // 0000 0001
    static const BYTE cmdop_Step20ms_c           = 0x02;   // 0000 0010
    static const BYTE cmdop_Step30ms_c           = 0x03;   // 0000 0011

    ///
    /// V - Track Verify
    /// ===============================================
    /// 0 - No verify
    /// 1 - Verify on destination
    ///
    static const BYTE cmdop_VerifyTrack_c        = 0x04;

    ///
    /// h - Head Load Flag
    /// ===============================================
    /// 0 - Load Head at the beginning
    /// 1 - Unload head at the beginning
    ///
    static const BYTE cmdop_HeadLoad_c           = 0x08;

    ///
    /// T - Track Update Flag
    /// ===============================================
    /// 0 - No update
    /// 1 - Update Track Register
    ///
    static const BYTE cmdop_TrackUpdate_c        = 0x10;

    ///
    /// m - Multiple Record Flag
    /// ===============================================
    /// 0 - Single Record
    /// 1 - Multiple records
    ///
    static const BYTE cmdop_MultipleRecord_c     = 0x10;

    ///
    /// a - Data Address Mark
    /// ===============================================
    /// 0 - FB(DAM)
    /// 1 - F8(delete DAM)
    ///
    static const BYTE cmdop_DataAddressMark_c    = 0x01;

    ///
    /// U - Update SSO
    /// ===============================================
    /// 0 - Update SSO to 0
    /// 1 - Update SSO to 1
    ///
    static const BYTE cmdop_UpdateSSO_c          = 0x02;
    static const BYTE cmdop_UpdateSSO_Shift_c    = 1;

    ///
    /// E - 15 mSec Delay
    /// ===============================================
    /// 0 - No delay
    /// 1 - 15 mSec Delay
    ///
    static const BYTE cmdop_Delay_15ms_c         = 0x04;

    ///
    /// L - Sector Length Flag
    ///---------------------------------------
    ///     | LSB in Sector Length in ID field
    ///     |   00   |   01   |   10  |   11
    /// ----+--------+--------+-------+-------
    /// L=0 |  256   |  512   |  1024 |  128
    /// L=1 |  128   |  256   |   512 | 1024
    ///
    static const BYTE cmdop_SectorLength_c       = 0x08;

    ///
    /// Options to the Force Interrupt command
    ///
    ///  Ix
    /// ===============================================
    ///  I3 - Immediate Interrupt, Requires a Reset
    ///  I2 - Index Pulse
    ///  I1 - Ready to Not Ready Transition
    ///  I0 - Not Ready to Ready Transition
    ///
    static const BYTE cmdop_ImmediateInterrupt_c = 0x08;
    static const BYTE cmdop_IndexPulse_c         = 0x04;
    static const BYTE cmdop_ReadyToNotReady_c    = 0x02;
    static const BYTE cmdop_NotReadyToReady_c    = 0x01;

    ///
    /// Status bit definitions
    ///

    /// Type I commands Status
    /// ===============================================
    static const BYTE stat_NotReady_c           = 0x80;
    static const BYTE stat_WriteProtect_c       = 0x40;
    static const BYTE stat_HeadLoaded_c         = 0x20;
    static const BYTE stat_SeekError_c          = 0x10;
    static const BYTE stat_CRCError_c           = 0x08;
    static const BYTE stat_TrackZero_c          = 0x04;
    static const BYTE stat_IndexPulse_c         = 0x02;
    static const BYTE stat_Busy_c               = 0x01;

    /// Read Address Status
    /// ===============================================
    /// stat_NotReady_c       - 0x80;
    /// 0                     - 0x40;
    /// 0                     - 0x20;
    static const BYTE stat_RecordNotFound_c     = 0x10;
    /// stat_CRCError_c       - 0x08;
    static const BYTE stat_LostData_c           = 0x04;
    static const BYTE stat_DataRequest_c        = 0x02;
    /// stat_Busy_c           - 0x01;

    /// Read Sector Status
    /// ===============================================
    /// stat_NotReady_c       - 0x80;
    /// 0                     - 0x40;
    static const BYTE stat_RecordType_c         = 0x20;
    /// stat_RecordNotFound_c - 0x10;
    /// stat_CRCError_c       - 0x08;
    /// stat_LostData_c       - 0x04;
    /// stat_DataRequest_c    - 0x02;
    /// stat_Busy_c           - 0x01;

    /// Read Track Status
    /// ===============================================
    /// stat_NotReady_c       - 0x80;
    /// 0                     - 0x40;
    /// 0                     - 0x20;
    /// 0                     - 0x10;
    /// 0                     - 0x08;
    /// stat_LostData_c       - 0x04;
    /// stat_DataRequest_c    - 0x02;
    /// stat_Busy_c           - 0x01;

    /// Write Sector Status
    /// ===============================================
    /// stat_NotReady_c       - 0x80;
    /// stat_WriteProtect_c   - 0x40;
    static const BYTE stat_WriteFault_c         = 0x20;
    /// stat_RecordNotFound_c - 0x10;
    /// stat_CRCError_c       - 0x08;
    /// stat_LostData_c       - 0x04;
    /// stat_DataRequest_c    - 0x02;
    /// stat_Busy_c           - 0x01;

    /// Write Track Status
    /// ===============================================
    /// stat_NotReady_c       - 0x80;
    /// stat_WriteProtect_c   - 0x40;
    /// stat_WriteFault_c     - 0x20;
    /// 0                     - 0x10;
    /// 0                     - 0x08;
    /// stat_LostData_c       - 0x04;
    /// stat_DataRequest_c    - 0x02;
    /// stat_Busy_c           - 0x01;

    static const int sectorLengths[2][4];
  private:
    void transferData(int data);
    bool checkAddr(BYTE addr[6]);
    int sectorLen(BYTE addr[6]);
    void updateReady(GenericFloppyDrive *drive);
};

#endif // WD1797_H_
