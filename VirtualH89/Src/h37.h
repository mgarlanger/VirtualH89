/// \file h37.h
///
/// \date May 13, 2009
/// \author Mark Garlanger
///

#ifndef Z_89_37_H_
#define Z_89_37_H_

#include "DiskController.h"
#include "WD179xUserIf.h"

#include "propertyutil.h"

class GenericFloppyDrive;
class WD1797;
class InterruptController;
class Computer;

///
/// \brief Virtual soft-sectored disk controller
///
/// A virtual Heathkit soft-sectored disk controller (Z-89-37).
///
/// The Z-89-37 uses the 1797-02 controller.
///
class Z_89_37: public DiskController, public WD179xUserIf
{
  public:
    Z_89_37(Computer* computer, int baseAddr, InterruptController* ic);
    virtual ~Z_89_37() override;

    static Z_89_37* install_H37(Computer*                   computer,
                                BYTE                        baseAddr,
                                InterruptController*        ic,
                                PropertyUtil::PropertyMapT& props,
                                std::string                 slot);
    virtual BYTE in(BYTE addr) override;
    virtual void out(BYTE addr,
                     BYTE val) override;

    virtual bool connectDrive(BYTE                unitNum,
                              GenericFloppyDrive* drive);

    virtual GenericFloppyDrive* getCurrentDrive();

    virtual bool removeDrive(BYTE unitNum);

    virtual void reset(void) override;

    // TODO: implement this
    std::vector<GenericDiskDrive*> getDiskDrives() override
    {
        return *(new std::vector<GenericDiskDrive*>());
    }
    std::string getDeviceName() override
    {
        return "H37";
    }
    GenericDiskDrive* findDrive(std::string ident) override
    {
        return nullptr;
    }
    std::string getDriveName(int index) override
    {
        return "";
    }
    std::string dumpDebug() override
    {
        return "";
    }
    virtual void raiseIntrq() override;
    virtual void raiseDrq() override;
    virtual void lowerIntrq() override;
    virtual void lowerDrq() override;
    virtual bool readReady() override;
    virtual int getClockPeriod() override;

  private:
    void motorOn(bool motor);

    Computer*            computer_m;
    WD1797*              wd1797_m;
    InterruptController* ic_m;

    static const BYTE    H37_NumPorts_c = 4;

    ///
    /// DK.PORT  - Octal - 0170 (0x78) base port.
    ///
    // DK.PORT
    static const BYTE BasePort_c                = 0x78;

    // DK.CON
    static const BYTE ControlPort_Offset_c      = 0;

    // DK.INT
    static const BYTE InterfaceControl_Offset_c = 1;

    // FD.STAT
    static const BYTE StatusPort_Offset_c       = 2;

    // FD.CMD
    static const BYTE CommandPort_Offset_c      = 2;

    // FD.DAT
    static const BYTE DataPort_Offset_c         = 3;

    // FD.SEC
    static const BYTE SectorPort_Offset_c       = 2;

    // FD.TRK
    static const BYTE TrackPort_Offset_c        = 3;

    BYTE              interfaceReg_m;
    BYTE              controlReg_m;
    bool              motorOn_m;

    bool              dataReady_m;
    bool              lostDataStatus_m;

    enum Disks
    {
        ds0        = 0,
        ds1        = 1,
        ds2        = 2,
        ds3        = 3,
        numDisks_c = 4
    };

    bool                sectorTrackAccess_m;

    GenericFloppyDrive* genericDrives_m[numDisks_c];
    Disks               curDiskDrive_m;

    bool                intrqAllowed_m;
    bool                drqAllowed_m;

    unsigned long long  cycleCount_m;


    /// Bits set in cmd_ControlPort_c - DK.CON
    enum ControlBits : BYTE
    {
        ctrl_EnableIntReq_c    = 0x01,
        ctrl_EnableDrqInt_c    = 0x02,
        ctrl_SetMFMRecording_c = 0x04,
        ctrl_MotorsOn_c        = 0x08,
        ctrl_Drive_0_c         = 0x10,
        ctrl_Drive_1_c         = 0x20,
        ctrl_Drive_2_c         = 0x40,
        ctrl_Drive_3_c         = 0x80
    };

    /// Bits to set alternate registers on InterfaceControl_c - DK.INT
    enum InterfaceControlBits : BYTE
    {
        if_SelectCommandData_c = 0x00,
        if_SelectSectorTrack_c = 0x01
    };

    /// Floppy disk related items
    // static const int    bytesPerTrack_c        = 6400;
    // static const int    clocksPerByte_c        = 64;
};

#endif // Z_89_37_H_
