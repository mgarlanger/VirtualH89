/// \file GenericSASIDrive.h
///
/// Implementation of a generic SASI disk drive and controller. Media is separate.
/// Currently based on a XEBEC controller (only documentation available).
///
/// \date  Feb 1, 2016
/// \author Douglas Miller
///

#ifndef GENERICSASIDRIVE_H_
#define GENERICSASIDRIVE_H_

#include "GenericDiskDrive.h"

#include "h89Types.h"


/// \cond
#include <memory>
/// \endcond

class GenericFloppyDisk;

///
/// \brief Virtual Generic SASI Drive
///
/// Implements a virtual floppy disk drive. Supports 48/96 tpi 5.25",
/// 48 tpi 8", either can be SS or DS. Note, the media determines density.
///
class GenericSASIDrive: public GenericDiskDrive
{
  public:
    enum DriveType
    {
        INVALID = 0,
        // TODO: fix this: controllers vs. drives...
        XEBEC_ST506,  //  5.3MB (XEBEC ctrlr + Seagate ST506 drive)
        XEBEC_ST412,  // 10.6MB
        XEBEC_CM5206, //  4.4MB
        XEBEC_CM5410, //  8.9MB
        XEBEC_CM5616, // 13.3MB
        XEBEC_RO201,  //  5.5MB
        XEBEC_RO202,  // 11.1MB
        XEBEC_RO203,  // 16.7MB
        XEBEC_RO204,  // 22.3MB
        other_other,  // not supported
        NUM_DRV_TYPE
    };

    GenericSASIDrive(DriveType   type,
                     std::string media,
                     int         cnum,
                     int         sectorSize);
    virtual ~GenericSASIDrive() override;
    static GenericSASIDrive* getInstance(std::string type,
                                         std::string path,
                                         int         unitNum);

    // not used:
    int getRawBytesPerTrack() override
    {
        return 0;
    }
    int getNumTracks() override
    {
        return 0;
    }
    bool isReady() override
    {
        return true;
    }
    bool isWriteProtect() override
    {
        return false;
    }
    void insertDisk(std::shared_ptr<GenericFloppyDisk> disk) override
    {
    }

    std::string getMediaName() override;

    // These are called by the host to indicate change events on ctl bits.
    void resetSASI(BYTE& data,
                   BYTE& dataOut,
                   BYTE& ctl);
    void select(BYTE& data,
                BYTE& dataOut,
                BYTE& ctl);
    void run(BYTE& dataIn,
             BYTE& dataOut,
             BYTE& ctl); // this is actually the absense of any bits.
    void ack(BYTE& dataIn,
             BYTE& dataOut,
             BYTE& ctl);
    void reset(BYTE& dataIn,
               BYTE& dataOut,
               BYTE& ctl);
    void deselect(BYTE& dataIn,
                  BYTE& dataOut,
                  BYTE& ctrl);

    int getCtlBits();
    void setCtlBits(int bits);
    void clearCtlBits(int bits);

    static const BYTE ctl_Busy_o_c         = 0x01;
    static const BYTE ctl_Ack_i_c          = 0x02;
    static const BYTE ctl_Reset_i_c        = 0x04;
    static const BYTE ctl_Msg_o_c          = 0x08;
    static const BYTE ctl_Sel_i_c          = 0x10;
    static const BYTE ctl_Cmd_o_c          = 0x20;
    static const BYTE ctl_Req_o_c          = 0x40;
    static const BYTE ctl_Out_o_c          = 0x80;

    // SASI Command codes, class/opcode.
    static const BYTE cmd_TestDriveReady_c = 0x00;
    static const BYTE cmd_Recal_c          = 0x01;
    static const BYTE cmd_ReqSense_c       = 0x03;
    static const BYTE cmd_FormatDrive_c    = 0x04;
    static const BYTE cmd_CheckTrackFmt_c  = 0x05;
    static const BYTE cmd_FormatTrack_c    = 0x06;
    static const BYTE cmd_FormatBadTrack_c = 0x07;
    static const BYTE cmd_Read_c           = 0x08;
    static const BYTE cmd_Write_c          = 0x0a;
    static const BYTE cmd_Seek_c           = 0x0b;
    static const BYTE cmd_InitDriveChar_c  = 0x0c;
    static const BYTE cmd_ReadECCLen_c     = 0x0d;
    static const BYTE cmd_FormatAltTrack_c = 0x0e;
    static const BYTE cmd_WriteSecBuf_c    = 0x0f;
    static const BYTE cmd_ReadSecBuf_c     = 0x10;
    static const BYTE cmd_RAMDiag_c        = 0xe0;
    static const BYTE cmd_DriveDiag_c      = 0xe3; // was 0xe4 - conflict!
    static const BYTE cmd_CtrlIntDiag_c    = 0xe4; // conflict!
    static const BYTE cmd_ReadLong_c       = 0xe5;
    static const BYTE cmd_WriteLong_c      = 0xe6;

  private:
    bool checkHeader(BYTE* b, int n);
    void startStatus(BYTE& ctrl);
    void startSense(BYTE& ctrl);
    void startDataIn(BYTE& ctrl);
    void startDataOut(BYTE& ctrl);
    void startError(BYTE& ctrl,
                    BYTE  err);
    void startDCB(BYTE& ctrl);
    void processCmd(BYTE& dataIn,
                    BYTE& dataOut,
                    BYTE& ctrl);
    void processData(BYTE& dataIn,
                     BYTE& dataOut,
                     BYTE& ctrl);
    void processDCB(BYTE& dataIn,
                    BYTE& dataOut,
                    BYTE& ctrl);

    enum State
    {
        IDLE,
        COMMAND,
        DATA_IN,
        DATA_OUT,
        SENSE,
        DRIVECB,
        STATUS,
    };
    enum State       curState;

    int              driveFd;
    int              driveSecLen;
    int              sectorsPerTrack;
    unsigned long    capacity;
    off_t            dataOffset;
    BYTE             driveCode;
    int              driveCnum;
    const char*      driveMedia;
    enum DriveType   driveType;
    long             mediaSpt;
    long             mediaSsz;
    long             mediaCyl;
    long             mediaHead;
    long             mediaLat;

    int              blockCount;

    // mode COMMAND
    BYTE             cmdBuf[6];
    static const int cmdLength = 6;
    int              cmdIx;
    // mode SENSE
    BYTE             senseBuf[4];
    static const int senseLength = 4;
    int              senseIx;
    // mode DRIVECB
    BYTE             dcbBuf[8];
    static const int dcbLength = 8;
    int              dcbIx;
    // mode STATUS
    BYTE             stsBuf[2];
    static const int stsLength = 2;
    static const int lastStsIx = 1;
    int              stsIx;
    // mode DATA_IN/DATA_OUT
    BYTE*            dataBuf;
    int              dataLength;
    int              dataIx;

    static int       params[NUM_DRV_TYPE][4];
};

#endif // GENERICSASIDRIVE_H_
