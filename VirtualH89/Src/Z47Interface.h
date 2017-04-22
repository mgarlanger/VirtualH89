///
/// \name Z47Interface.h
///
/// Interface card on the H89 to support the Z47 external dual 8" floppy disks.
///
/// \date Jul 14, 2013
/// \author Mark Garlanger
///

#ifndef Z47INTERFACE_H_
#define Z47INTERFACE_H_

#include "DiskController.h"
#include "ClockUser.h"
#include "ParallelPortConnection.h"

class Z47Interface: public virtual DiskController,
                    public virtual ClockUser,
                    public virtual ParallelPortConnection
{
  public:
    Z47Interface(int baseAddr);
    virtual ~Z47Interface() override;

    virtual BYTE in(BYTE addr) override;
    virtual void out(BYTE addr,
                     BYTE val) override;

    virtual void reset(void) override;
    virtual void notification(unsigned int cycleCount) override;
    void connectDriveLink(ParallelLink* link);

    virtual void raiseSignal(SignalType sigType) override;
    virtual void lowerSignal(SignalType sigType) override;
    virtual void pulseSignal(SignalType sigType) override;

    // TODO: implement this
    std::vector<GenericDiskDrive*> getDiskDrives() override
    {
        return *(new std::vector<GenericDiskDrive*>());
    }
    std::string getDeviceName() override
    {
        return "Z47";
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

  private:
    static const BYTE H47_NumPorts_c = 2;

    ///
    /// 0170 (0x78) base port. (or could be 0174) if in the right-hand slot
    /// ? move to public?
    static const BYTE BasePort1_c                = 0x78;
    static const BYTE BasePort2_c                = 0x7c;

    static const BYTE StatusPort_Offset_c        = 0;
    static const BYTE DataPort_Offset_c          = 1;


    static const BYTE stat_Error_c               = 0x01;
    static const BYTE stat_sw101_a_c             = 0x02;
    static const BYTE stat_sw101_b_c             = 0x04;
    static const BYTE stat_sw101_c_c             = 0x08;
    static const BYTE stat_sw101_d_c             = 0x10;
    static const BYTE stat_Done_c                = 0x20;
    static const BYTE stat_InterruptEnabled_c    = 0x40;
    static const BYTE stat_DataTransferRequest_c = 0x80;

    //
    // Commands
    //
    static const BYTE cmd_U137BSet_c          = 0x01;
    static const BYTE cmd_MasterReset_c       = 0x02;
    static const BYTE cmd_InterruptsEnabled_c = 0x40;
    static const BYTE cmd_Undefined_c         = ~(cmd_U137BSet_c |
                                                  cmd_MasterReset_c |
                                                  cmd_InterruptsEnabled_c);

    bool              interruptsEnabled_m;
    bool              DTR_m;
    bool              DDOut_m;
    bool              Busy_m;
    bool              Error_m;
    bool              Done_m;

    void writeStatus(BYTE cmd);
    void writeData(BYTE data);

    void readStatus(BYTE& status);
    void readData(BYTE& data);

    ParallelLink* linkToDrive_m;
};

#endif // Z47INTERFACE_H_
