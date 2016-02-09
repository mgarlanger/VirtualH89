/// \file h17.h
///
/// Hard-sectored Disk Controller
///
/// \date Apr 18, 2009
/// \author Mark Garlanger
///

#ifndef H17_H_
#define H17_H_

#include "config.h"
#include "h89Types.h"
#include "DiskController.h"
#include "ClockUser.h"
#include <vector>
#include <string>

class DiskDrive;

///
/// \brief %H17 - Virtual Hard-sectored Disk Controller
///
/// A virtual hard-sectored disk controller (H-88-1), supports up to 3 disk drives (H-17-1)
/// at 102k per disk. Later (third-party) software upgrades supported disks up to 408k per
/// disk, by using a double-sided 96 tpi drive (H-17-4 or H-17-5).
///
class H17 : public DiskController, public ClockUser
{
  public:
    H17(int BaseAddr);
    virtual ~H17();

    virtual BYTE in(BYTE addr);
    virtual void out(BYTE addr, BYTE val);

    virtual bool connectDrive(BYTE unitNum, DiskDrive *drive);
    virtual bool removeDrive(BYTE unitNum);

    virtual void selectSide(BYTE side);

    virtual void notification(unsigned int cycleCount);

    // TODO: implement this
    std::vector<GenericDiskDrive *> getDiskDrives()
    {
        return *(new std::vector<GenericDiskDrive *>());
    }
    std::string getDeviceName()
    {
        return "H17";
    }

  private:

    enum State
    {
        idleState,
        seekingSyncState,
        readingState,
        writingState
    };
    State state_m;

    // for the spinning of the disk
    unsigned long long spinCycles_m;

    unsigned long curCharPos_m;

    bool motorOn_m;
    bool writeGate_m;
    bool direction_m;

    // statuses
    bool syncCharacterReceived_m;
    bool receiveDataAvail_m;
    bool receiverOverrun_m;
//  bool receiverParityErr_m; // Not needed based the the H17 circuit, no parity is used.
    bool fillCharTransmitted_m;
    bool transmitterBufferEmpty_m;

    BYTE receiverOutputRegister_m;
    BYTE transmitterHoldingRegister_m;

    enum DiskDriveID
    {
        ds0 = 0,
        ds1 = 1,
        ds2 = 2,
        maxDiskDrive_c = 3
    };

    DiskDriveID curDrive_m;

    DiskDrive *drives_m[maxDiskDrive_c];

    BYTE fillChar_m;
    BYTE syncChar_m;

    ///
    /// Ports
    ///
    static const BYTE H17_NumPorts_c       = 4;

    ///
    /// Data Port - Typically Port 0x7c (Octal 0174)
    ///
    /// Receives and transmits the data characters to the USART
    ///
    static const BYTE DataPortOffset_c     = 0;

    ///
    /// Status Port (read) - Typically Port 0x7d (Octal 0175)
    /// Fill Port (write)  - Typically Port 0x7d (Octal 0175)
    ///
    /// Reads the USART status, writes set the fill character.
    ///
    static const BYTE StatusPortOffset_c   = 1;
    static const BYTE FillPortOffset_c     = 1;

    ///
    /// Sync Port - Typically Port 0x7e (Octal 0176)
    ///
    /// Reads resets the USART to search character mode
    /// Writes the search character to the USART
    ///
    static const BYTE SyncPortOffset_c     = 2;

    ///
    /// Control Port - Typically Port 0x7f (Octal 0177)
    ///
    /// Reads the floppy disk status and sync character match and controls the floppy disk
    /// with respect to it's motor, track position, and write gate.
    ///
    static const BYTE ControlPortOffset_c  = 3;

    ///
    /// Number of CPU cycles for each read byte from the Floppy disk.
    /// This is fixed on due to the single density, and rotation speed of the disk
    /// \todo move this to a disk or disk drive class.
    static const unsigned int CPUCyclesPerByte_c = 128;
    static const unsigned int BytesPerTrack_c    = 3200;

    /// Flag and Control Bits
    ///
    /// Read from ControlPort_c
    ///
    static const BYTE ctrlHoleDetect_Flag         = 0x01; // from floppy disk
    static const BYTE ctrlTrackZeroDetect_Flag    = 0x02; // from disk drive
    static const BYTE ctrlWriteProtect_Flag       = 0x04; // from floppy disk
    static const BYTE ctrlSyncDetect_Flag         = 0x08; // from controller

    ///
    /// Write to ControlPort_c
    ///
    static const BYTE WriteGate_Ctrl              = 0x01;
    static const BYTE DriveSelect0_Ctrl           = 0x02;
    static const BYTE DriveSelect1_Ctrl           = 0x04;
    static const BYTE DriveSelect2_Ctrl           = 0x08;
    static const BYTE MotorOn_Ctrl                = 0x10;  // Controls all the drives
    static const BYTE Direction_Ctrl              = 0x20;  // (0 = out)
    static const BYTE StepCommand_Ctrl            = 0x40;  // (Active high)
    static const BYTE WriteEnableRAM_Ctrl         = 0x80;  // 0 - write protected
    // 1 - write enabled

    ///
    /// Read from the StatusPort_c
    ///
    static const BYTE ReceiveDataAvail_Flag       = 0x01;  // from controller
    static const BYTE ReceiverOverrun_Flag        = 0x02;  // from controller
    static const BYTE ReceiverParityErr_Flag      = 0x04;  // from controller, NEVER set
    // on H17
    static const BYTE SyncDetect_Flag             = 0x08;  // ??? - Manual says it, but Heath
    // software didn't use it
    static const BYTE FillCharTransmitted_Flag    = 0x40;  // from controller
    static const BYTE TransmitterBufferEmpty_Flag = 0x80;  // from controller
};

#endif // H17_H_
