/// \file mms77320.h
///
/// Implementation of the MMS 77320 SASI adapter. Does not implement the SASI controller
/// or disk drive (or media), see GenericSASIDrive et al.
///
/// \date Feb 13, 2016
/// \author Douglas Miller
///

#ifndef MMS77320_H_
#define MMS77320_H_


#include "DiskController.h"
#include "propertyutil.h"

class GenericSASIDrive;

///
/// \brief Virtual soft-sectored disk controller
///
/// A virtual Magnolia Microsystems soft-sectored disk controller (MMS77320).
/// Note: this is NOT complete or even marginally functional.
///
/// The MMS77320 uses the 1797-02 controller.
///
class MMS77320: public DiskController
{
  public:
    static const int numDisks_c = 8;

    MMS77320(int baseAddr,
             int intLevel,
             int switches);
    virtual ~MMS77320();

    static MMS77320* install_MMS77320(PropertyUtil::PropertyMapT& props,
                                      std::string                 slot);

    virtual BYTE in(BYTE addr);
    virtual void out(BYTE addr,
                     BYTE val);

    virtual bool connectDrive(BYTE              unitNum,
                              GenericSASIDrive* drive);
    virtual bool removeDrive(BYTE unitNum);
    virtual GenericSASIDrive* getDrive(BYTE unitNum);

    virtual void reset(void);

    // TODO: implement this
    std::vector<GenericDiskDrive*> getDiskDrives();
    std::string getDriveName(int index);
    std::string getDeviceName()
    {
        return MMS77320_Name_c;
    }
    GenericDiskDrive* findDrive(std::string ident);
    std::string dumpDebug();

    bool interResponder(BYTE& opCode);

  protected:
    static const char* MMS77320_Name_c;

  private:
    void raiseIntrq();
    void lowerIntrq();

    static const BYTE MMS77320_NumPorts_c   = 3;

    BYTE              BasePort_m            = 0xff; // TBD configured by jumpers
    static const BYTE DataPort_Offset_c     = 0;
    static const BYTE Control0Port_Offset_c = 1;
    static const BYTE StatusPort_Offset_c   = 1;
    static const BYTE SwitchPort_Offset_c   = 2;
    static const BYTE Control1Port_Offset_c = 2;
    BYTE              dataOutReg_m;
    BYTE              dataInReg_m;
    BYTE              control0Reg_m;
    BYTE              control1Reg_m;
    BYTE              statusReg_m;
    BYTE              switchReg_m;
    BYTE              ctrlBus_m;

    BYTE getStatus(BYTE ctl);
    GenericSASIDrive* getCurDrive();
    GenericSASIDrive* drives_m[numDisks_c];
    GenericSASIDrive* curDrive_m;

    unsigned char     intLevel_m; // TBD configured by jumpers

    /// Bits set in cmd_Control0Port_c
    static const BYTE ctrl_ResetStart_c   = 0x10;
    static const BYTE ctrl_EnableIntReq_c = 0x20;
    static const BYTE ctrl_Select_c       = 0x40;
    static const BYTE ctrl_DisableAck_c   = 0x80;
    /// Bits set in cmd_Control1Port_c
    static const BYTE ctrl_DriveSel_c     = 0x07;

    // Bits in statusReg_m
    static const BYTE sts_Req_c           = 0x80;
    static const BYTE sts_POut_c          = 0x40;
    static const BYTE sts_Msg_c           = 0x20;
    static const BYTE sts_Cmd_c           = 0x10;
    static const BYTE sts_Busy_c          = 0x08;
    static const BYTE sts_Unused_c        = 0x04;
    static const BYTE sts_ActInt_c        = 0x02;
    static const BYTE sts_Ack_c           = 0x01;

    bool intrqAllowed()
    {
        return (control0Reg_m & ctrl_EnableIntReq_c) != 0;
    }
};

#endif // MMS77320_H_
