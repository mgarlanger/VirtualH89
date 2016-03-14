/// \file GenericSASIDrive.cpp
///
/// Implementation of a generic SASI disk drive and controller. Media is separate.
/// Currently based on a XEBEC controller (only documentation available).
///
/// \date Feb 2, 2016
/// \author Douglas Miller
///

#include "GenericSASIDrive.h"

#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <unistd.h>
#include <string.h>
#include <stdlib.h>
#include <errno.h>

#include "logger.h"

/* Expected drive characteristic by drive: (XEBEC S1410 manual)
 * Drive        cyl  hd red-wr precomp
 * CM5206   256  2  256 256
 * CM5410   256  4  256 256
 * CM5616   256  6  256 256
 * HD561    180  2  128    180
 * HD562    180  2  128    180
 * RMS503   153  2  77  77
 * RMS506   153  4  77  77
 * RMS512   153  8  77  77
 * ST506    153  4  128 64
 * ST412    306  4  128 64
 * TM602S   153  4  128 153
 * TM603S   153  6  128 153
 * TM603SE  230  6  128 128
 * TI5.25+  153  4  64  64
 * RO101    192  2  96  0
 * RO102    192  4  96  0
 * RO103    192  6  96  0
 * RO104    192  8  96  0
 * RO201    321  2  132 0
 * RO202    321  4  132 0
 * RO203    321  6  132 0
 * RO204    321  8  132 0
 * MS1-006  306  2  153 0
 * MS1-012  306  4  153 0
 */
// Grrr... C++ and no designated initializers
#if 0
int GenericSASIDrive::params[NUM_DRV_TYPE][4]
{
    [XEBEC_ST506]  = {153, 4, 128, 64},
    [XEBEC_ST412]  = {306, 4, 128, 64},
    [XEBEC_CM5206] = {256, 2, 256, 256},
    [XEBEC_CM5410] = {256, 4, 256, 256},
    [XEBEC_CM5616] = {256, 6, 256, 256},
    [XEBEC_RO201]  = {321, 2, 132, 0},
    [XEBEC_RO202]  = {321, 4, 132, 0},
    [XEBEC_RO203]  = {321, 6, 132, 0},
    [XEBEC_RO204]  = {321, 8, 132, 0},
};
#endif
int GenericSASIDrive::params[NUM_DRV_TYPE][4]
{
    /* INVALID=0    */ {0, 0, 0, 0},
    /*[XEBEC_ST506] */ {153, 4, 128, 64},
    /*[XEBEC_ST412] */ {306, 4, 128, 64},
    /*[XEBEC_CM5206]*/ {256, 2, 256, 256},
    /*[XEBEC_CM5410]*/ {256, 4, 256, 256},
    /*[XEBEC_CM5616]*/ {256, 6, 256, 256},
    /*[XEBEC_RO201] */ {321, 2, 132, 0},
    /*[XEBEC_RO202] */ {321, 4, 132, 0},
    /*[XEBEC_RO203] */ {321, 6, 132, 0},
    /*[XEBEC_RO204] */ {321, 8, 132, 0},
};

bool
GenericSASIDrive::checkHeader(BYTE* buf, int n)
{
    BYTE* b = buf;
    int   m = 0;

    while (*b != '\n' && *b != '\0' && b - buf < n)
    {
        BYTE* e;
        int   p = strtoul((char*) b, (char**) &e, 0);

        // TODO: removable flag, others?
        // NOTE: removable media requires many more changes.
        switch (tolower(*e))
        {
            case 'c':
                m       |= 0x01;
                mediaCyl = p;
                break;

            case 'h':
                m        |= 0x02;
                mediaHead = p;
                break;

            case 'z':
                m       |= 0x04;
                mediaSsz = p;
                break;

            case 'p':
                m       |= 0x08;
                mediaSpt = p;
                break;

            case 'l':
                m       |= 0x10;
                mediaLat = p;
                break;

            default:
                return false;
        }

        b = e + 1;
    }

    return (m == 0x1f);
}

GenericSASIDrive::GenericSASIDrive(DriveType   type,
                                   std::string media,
                                   int         cnum,
                                   int         sectorSize) :
       curState(IDLE),
       driveFd(-1),
       driveSecLen(0),
       sectorsPerTrack(0),
       capacity(0),
       dataOffset(0),
       driveCode(0),
       driveCnum(0),
       driveMedia(NULL),
       driveType(INVALID),
       mediaSpt(0),
       mediaSsz(0),
       mediaCyl(0),
       mediaHead(0),
       mediaLat(0),
       cmdIx(0),
       senseIx(0),
       dcbIx(0),
       stsIx(0),
       dataBuf(NULL),
       dataLength(0),
       dataIx(0)
{
    driveType   = type;
    driveMedia  = strdup(media.c_str());
    driveCnum   = cnum;
    driveSecLen = sectorSize;

    if (sectorSize == 256)
    {
        sectorsPerTrack = 32;
    }
    else if (sectorSize == 512)
    {
        sectorsPerTrack = 17;
    }
    else
    {
        // not supported, but do something...
        if (sectorSize == 0)
        {
            sectorsPerTrack = 0;
        }
        else
        {
            sectorsPerTrack = 8192 / sectorSize;
        }
    }

    capacity   = params[type][0] * params[type][1] * sectorsPerTrack * sectorSize;
    dataBuf    = new BYTE[sectorSize + 4]; // space for ECC for "long" commands
    dataLength = sectorSize;

    driveFd    = open(driveMedia, O_RDWR | O_CREAT, 0666);

    if (driveFd < 0)
    {
        debugss(ssMMS77320, ERROR, "Unable to open media %s: %d\n", driveMedia, errno);
        return;
    }

    dataOffset = 0;
    BYTE  buf[128];
    off_t end = lseek(driveFd, (off_t) 0, SEEK_END);

    // special case: 0 (EOF) means new media - initialize it.
    if (end == 0)
    {
        mediaCyl  = params[type][0];
        mediaHead = params[type][1];
        mediaSsz  = sectorSize;
        mediaSpt  = sectorsPerTrack;
        mediaLat  = 1;
        memset(buf, 0, sizeof(buf));
        int l = sprintf((char*) buf, "%dc%dh%dz%dp%dl\n", mediaCyl, mediaHead,
                        mediaSsz, mediaSpt, mediaLat);
        // NOTE: 'buf' includes a '\n'...
        debugss(ssMMS77320, ERROR, "Initializing new media %s as %s", driveMedia, buf);
        ftruncate(driveFd, capacity + sizeof(buf));
        lseek(driveFd, capacity, SEEK_SET); // i.e. END - sizeof(buf)
        write(driveFd, buf, l);
    }
    else
    {
        // first, trying reading the last 128 bytes...
        lseek(driveFd, end - sizeof(buf), SEEK_SET);
        int  x    = read(driveFd, buf, sizeof(buf));
        bool done = (x == sizeof(buf) && checkHeader(buf, sizeof(buf)));

        if (!done)
        {
            lseek(driveFd, (off_t) 0, SEEK_SET);
            x          = read(driveFd, buf, sizeof(buf));
            done       = (x == sizeof(buf) && checkHeader(buf, sizeof(buf)));
            dataOffset = mediaSsz;
        }

        if (!done)
        {
            debugss(ssMMS77320, ERROR, "Bad media header: %s\n", driveMedia);
            close(driveFd);
            driveFd = -1;
            return;
        }

        if (mediaSpt != sectorsPerTrack || mediaSsz != driveSecLen ||
            params[type][0] != mediaCyl || params[type][1] != mediaHead)
        {
            // TODO: fatal or warning?
            debugss(ssMMS77320, ERROR, "Media/Drive mismatch: %s\n", driveMedia);
            close(driveFd);
            driveFd = -1;
            return;
        }

        debugss(ssMMS77320, ERROR, "Mounted existing media %s as %s", driveMedia, buf);
    }
}

GenericSASIDrive*
GenericSASIDrive::getInstance(std::string type,
                              std::string media,
                              int         cnum)
{
    DriveType etype;

    if (type.compare("XEBEC_ST506") == 0)
    {
        etype = XEBEC_ST506;
    }
    else if (type.compare("XEBEC_ST412") == 0)
    {
        etype = XEBEC_ST412;
    }
    else if (type.compare("XEBEC_CM5206") == 0)
    {
        etype = XEBEC_CM5206;
    }
    else if (type.compare("XEBEC_CM5410") == 0)
    {
        etype = XEBEC_CM5410;
    }
    else if (type.compare("XEBEC_CM5616") == 0)
    {
        etype = XEBEC_CM5616;
    }
    else if (type.compare("XEBEC_RO201") == 0)
    {
        etype = XEBEC_RO201;
    }
    else if (type.compare("XEBEC_RO202") == 0)
    {
        etype = XEBEC_RO202;
    }
    else if (type.compare("XEBEC_RO203") == 0)
    {
        etype = XEBEC_RO203;
    }
    else if (type.compare("XEBEC_RO204") == 0)
    {
        etype = XEBEC_RO204;
    }
    else
    {
        debugss(ssMMS77320, ERROR, "Invalid controller/drive combination: %s\n", type.c_str());
        return NULL;
    }

    /*
     * port determination based on SW501...
        324         MVI     B,7CH
        325         IN      GPIO            ; READ SWITCH 501
        326         ANI     00000011B       ; WHAT'S PORT 7C SET FOR ?
        327         CPI     00000010B       ;  IF Z67, THEN THIS IS IT
        328         JRZ     GOTPRT
        329         MVI     B,78H
        330         IN      GPIO            ; READ SWITCH 501
        331         ANI     00001100B       ; WHAT'S PORT 78 SET FOR ?
        332         CPI     00001000B       ;  IF Z67, THEN THIS IS IT
        333         RNZ
     */

//         DB      0,153,4,0,128,0,64,11   ; DRIVE CHARACTERISTIC DATA
//  =   0099, 04, 0080, 0064, 0b
//  =   153 cyl, 4 heads, 128 red-wr, 100 wr-pre, 11 ecc-burst
//  =   612 trk. (~16 spt, 512B ea, ?)
    // ssz = jumper_W2 ? 512 : 256; // XEBEC jumper "W2" a.k.a. "SS" pos "2" or "5"
    int ssz = 512;
    return new GenericSASIDrive(etype, media, cnum, ssz);
}

GenericSASIDrive::~GenericSASIDrive()
{
    if (driveFd < 0)
    {
        return;
    }

    close(driveFd);
    driveFd = -1;
}


/*
   Typical sequence:

   "Hail" controller:
                wait !BSY (while poking data reg)
                SEL
                wait BSY
                RUN

   Send command:
                wait until REQ+CMD+POUT+BSY
                out next cmd byte
                loop

   Read command                        Write command
               if loose POUT, goto status
                wait for !CMD (loop)
   inir 128 bytes                      outir 128 bytes
                loop

   Status phase:
                 check MSG+REQ+CMD+POUT
                 if REQ+CMD then input data and loop
                 if not MSG+REQ+CMD loop (no data read)
                 input (discard) data
                 return previous data byte (has error status)

   MSG    REQ    CMD    POUT    BSY
   ---------------------------------
   X      X      X      X       -       OK to begin
                                        SEL
   X      X      X      X      BSY      controller ready
                                        RUN

   X     REQ    CMD    POUT    BSY      OK to send command
                                        OUT cmd byte (ACK)
                                        loop

   X     REQ    CMD     -      BSY      done with data (status begins)
   X     REQ     -      X      BSY      OK to send/get data (bulk 128 bytes)
                                        IN/OUT data byte (ACK)

   -     REQ    CMD     -       X       OK get status byte
                                        IN status byte (ACK)

   MSG    REQ    CMD     -       X      OK last status byte (prev has error)
                                        IN status byte (ACK)
   presumably resturns to:
   X     REQ    CMD    POUT    BSY      OK to send command.

 */

void
GenericSASIDrive::deselect(BYTE& dataIn,
                           BYTE& dataOut,
                           BYTE& ctrl)
{
    // User is switch controllers/drive, do any cleanup.
    ctrl    &= ~ctl_Msg_o_c;
    ctrl    &= ~ctl_Cmd_o_c;
    ctrl    &= ~ctl_Out_o_c;
    ctrl    &= ~ctl_Req_o_c;
    ctrl    &= ~ctl_Busy_o_c;
    curState = IDLE;
}

void
GenericSASIDrive::ack(BYTE& dataIn,
                      BYTE& dataOut,
                      BYTE& ctrl)
{
    ctrl &= ~ctl_Req_o_c;
    ctrl &= ~ctl_Ack_i_c;

    if (curState == COMMAND)
    {
        cmdBuf[cmdIx++] = dataOut;

        if (cmdIx >= cmdLength)
        {
            curState   = IDLE; // default? should get set to something else.
            ctrl      &= ~ctl_Cmd_o_c;
            cmdIx      = 0;
            blockCount = 0;
            debugss(ssGenericSASIDrive, INFO, "Command: %02x %02x %02x %02x %02x %02x\n",
                    cmdBuf[0], cmdBuf[1], cmdBuf[2], cmdBuf[3], cmdBuf[4], cmdBuf[5]);
            // this further modifies 'ctrl' and sets curState.
            processCmd(dataIn, dataOut, ctrl);
            return;
        }

        ctrl |= ctl_Req_o_c;
        ctrl &= ~ctl_Ack_i_c;
    }
    else if (curState == STATUS)
    {
        if (stsIx < stsLength)
        {
            dataIn = stsBuf[stsIx++];

            if (stsIx >= stsLength)
            {
                ctrl |= ctl_Msg_o_c;
            }

            ctrl |= ctl_Req_o_c;
            ctrl &= ~ctl_Ack_i_c;
            return;
        }

        // must start over at SEL again...
        ctrl    &= ~ctl_Msg_o_c;
        ctrl    &= ~ctl_Busy_o_c; // it sppears this must go low...
        ctrl    &= ~ctl_Cmd_o_c;
        ctrl    &= ~ctl_Out_o_c;
        ctrl    &= ~ctl_Req_o_c;
        curState = IDLE;
    }
    else if (curState == SENSE)
    {
        if (senseIx < senseLength)
        {
            dataIn = senseBuf[senseIx++];
            ctrl  |= ctl_Req_o_c;
            ctrl  &= ~ctl_Ack_i_c;
            return;
        }

        senseIx = 0;
        startStatus(ctrl);
        ack(dataIn, dataOut, ctrl);

    }
    else if (curState == DATA_IN)
    {
        if (dataIx < dataLength)
        {
            dataIn = dataBuf[dataIx++];
            ctrl  |= ctl_Req_o_c;
            ctrl  &= ~ctl_Ack_i_c;
            return;
        }

        // We must either put some data in the reg or lower BSY...
        dataIx = 0;

        if (++blockCount < cmdBuf[4])
        {
            processCmd(dataIn, dataOut, ctrl);
            return;
        }

        startStatus(ctrl);
        ack(dataIn, dataOut, ctrl);

    }
    else if (curState == DRIVECB)
    {
        dcbBuf[dcbIx++] = dataOut;

        if (dcbIx >= dcbLength)
        {
            dcbIx = 0;
            processDCB(dataIn, dataOut, ctrl);
            return;
        }

        ctrl |= ctl_Req_o_c;
        ctrl &= ~ctl_Ack_i_c;
    }
    else if (curState == DATA_OUT)
    {
        dataBuf[dataIx++] = dataOut;

        if (dataIx >= dataLength)
        {
            dataIx = 0;
            processData(dataIn, dataOut, ctrl);
            return;
        }

        ctrl |= ctl_Req_o_c;
        ctrl &= ~ctl_Ack_i_c;
    }
    else
    {
        // must be IDLE, and host is trying to sync-up with us.
        // make everything look like we are not here...
        ctrl &= ~ctl_Msg_o_c;
        ctrl &= ~ctl_Cmd_o_c;
        ctrl &= ~ctl_Out_o_c;
        ctrl &= ~ctl_Req_o_c;
        ctrl &= ~ctl_Busy_o_c;
    }
}

void
GenericSASIDrive::resetSASI(BYTE& dataIn,
                            BYTE& dataOut,
                            BYTE& ctrl)
{
    ctrl    &= ~ctl_Msg_o_c;
    ctrl    &= ~ctl_Cmd_o_c;
    ctrl    &= ~ctl_Out_o_c;
    ctrl    &= ~ctl_Req_o_c;
    ctrl    &= ~ctl_Busy_o_c;
    curState = IDLE;
}

void
GenericSASIDrive::select(BYTE& dataIn,
                         BYTE& dataOut,
                         BYTE& ctrl)
{
    // validate data bit with cnum...
    int i = ffs(dataOut);

    if (i == 0)
    {
        return;
    }

    if (i - 1 == driveCnum)
    {
        ctrl |= ctl_Busy_o_c;
    }
}

void
GenericSASIDrive::run(BYTE& dataIn,
                      BYTE& dataOut,
                      BYTE& ctrl)
{
    ctrl    |= ctl_Busy_o_c;
    ctrl    |= ctl_Cmd_o_c;
    ctrl    |= ctl_Out_o_c;
    ctrl    |= ctl_Req_o_c;
    cmdIx    = 0;
    curState = COMMAND;
}

void
GenericSASIDrive::startStatus(BYTE& ctrl)
{
    stsIx    = 0;
    curState = STATUS;
    ctrl    &= ~ctl_Msg_o_c;
    ctrl    |= ctl_Cmd_o_c;
    ctrl    &= ~ctl_Out_o_c;
    ctrl    |= ctl_Req_o_c;
}

void
GenericSASIDrive::startSense(BYTE& ctrl)
{
    senseIx  = 0;
    curState = SENSE;
    ctrl    &= ~ctl_Msg_o_c;
    ctrl    |= ctl_Cmd_o_c;
    ctrl    &= ~ctl_Out_o_c;
    ctrl    |= ctl_Req_o_c;
}

void
GenericSASIDrive::startDataIn(BYTE& ctrl)
{
    dataIx   = 0;
    curState = DATA_IN;
    ctrl    &= ~ctl_Msg_o_c;
    ctrl    &= ~ctl_Cmd_o_c;
    ctrl    &= ~ctl_Out_o_c;
    ctrl    |= ctl_Req_o_c;
}

void
GenericSASIDrive::startDataOut(BYTE& ctrl)
{
    dataIx   = 0;
    curState = DATA_OUT;
    ctrl    &= ~ctl_Msg_o_c;
    ctrl    &= ~ctl_Cmd_o_c;
    ctrl    |= ctl_Out_o_c;
    ctrl    |= ctl_Req_o_c;
}

void
GenericSASIDrive::startError(BYTE& ctrl,
                             BYTE  err)
{
    stsBuf[0]   = 0b00000010;
    stsBuf[1]   = 0;
    senseBuf[0] = err;
    startStatus(ctrl);
}

void
GenericSASIDrive::startDCB(BYTE& ctrl)
{
    dcbIx    = 0;
    curState = DRIVECB;
    ctrl    &= ~ctl_Msg_o_c;
    ctrl    &= ~ctl_Cmd_o_c;
    ctrl    |= ctl_Out_o_c;
    ctrl    |= ctl_Req_o_c;
}

void
GenericSASIDrive::processCmd(BYTE& dataIn,
                             BYTE& dataOut,
                             BYTE& ctrl)
{
    off_t off;
    long  e;

    if (cmdBuf[0] != cmd_ReqSense_c)
    {
        memset(senseBuf, 0, 4);
    }

    switch (cmdBuf[0])
    {
        case cmd_TestDriveReady_c:
        case cmd_Recal_c:
        case cmd_RAMDiag_c:
        case cmd_DriveDiag_c:
        case cmd_CtrlIntDiag_c:
            // no-op: just return success
            stsBuf[0] = 0;
            stsBuf[1] = 0;
            stsIx     = 0;
            startStatus(ctrl);
            ack(dataIn, dataOut, ctrl);
            break;

        case cmd_ReqSense_c:
            // sense data was set by previous command...
            senseIx = 0;
            startSense(ctrl);
            ack(dataIn, dataOut, ctrl);
            break;

        case cmd_Read_c:
        case cmd_ReadLong_c:
            if (driveFd < 0)
            {
                startError(ctrl, 0x84); // drive not ready + addr valid
                ack(dataIn, dataOut, ctrl);
                break;
            }

            memcpy(senseBuf + 1, cmdBuf + 1, 3);
            off = ((((((cmdBuf[1] & 0x1f) << 8) | cmdBuf[2]) << 8) | cmdBuf[3]) +
                   blockCount) * driveSecLen;

            if (off >= capacity)
            {
                startError(ctrl, 0x21); // illegal disk address
                ack(dataIn, dataOut, ctrl);
                break;
            }

            lseek(driveFd, off + dataOffset, SEEK_SET);
            e = read(driveFd, dataBuf, dataLength);

            if (e != dataLength)
            {
                startError(ctrl, 0x94); // target sector not found + addr valid
                ack(dataIn, dataOut, ctrl);
                break;
            }

            dataLength = driveSecLen;

            if (cmdBuf[0] == cmd_ReadLong_c)
            {
                dataLength       += 4;
                // TODO: must we compute ECC?
                dataBuf[dataIx++] = 0;
                dataBuf[dataIx++] = 0;
                dataBuf[dataIx++] = 0;
                dataBuf[dataIx++] = 0;
            }

            startDataIn(ctrl);
            dataIx = 0;
            // a little risky to recurse here, but startDataIn() should
            // ensure we can't loop.
            ack(dataIn, dataOut, ctrl);
            break;

        case cmd_Write_c:
        case cmd_WriteLong_c:
            if (driveFd < 0)
            {
                startError(ctrl, 0x84); // drive not ready + addr valid
                ack(dataIn, dataOut, ctrl);
                break;
            }

            memcpy(senseBuf + 1, cmdBuf + 1, 3);
            dataLength = driveSecLen;

            if (cmdBuf[0] == cmd_ReadLong_c)
            {
                dataLength += 4;
            }

            dataIx = 0;
            // could validate address...
            startDataOut(ctrl);
            break;

        case cmd_WriteSecBuf_c:
            dataIx     = 0;
            dataLength = driveSecLen;
            startDataOut(ctrl);
            break;

        case cmd_ReadSecBuf_c:
            dataIx     = 0;
            dataLength = driveSecLen;
            startDataIn(ctrl);
            ack(dataIn, dataOut, ctrl);
            break;

        case cmd_InitDriveChar_c:
            startDCB(ctrl);
            break;

        case cmd_FormatDrive_c:
        case cmd_FormatTrack_c:
        case cmd_FormatBadTrack_c:
        case cmd_CheckTrackFmt_c:
        case cmd_Seek_c:
        case cmd_FormatAltTrack_c:
            // validate address, but otherwise just return success.
            blockCount = 0;
            off        = ((((((cmdBuf[1] & 0x1f) << 8) | cmdBuf[2]) << 8) | cmdBuf[3]) +
                          blockCount) * driveSecLen;

            if (off >= capacity)
            {
                startError(ctrl, 0x21); // illegal disk address
                ack(dataIn, dataOut, ctrl);
                break;
            }

            stsBuf[0] = 0;
            stsBuf[1] = 0;
            stsIx     = 0;
            startStatus(ctrl);
            ack(dataIn, dataOut, ctrl);
            break;

        case cmd_ReadECCLen_c:
            // TODO: transfer 1 data byte, but this command is only valid
            // after a correctable ECC data error 0x18, which we never return.
            stsBuf[0] = 0;
            stsBuf[1] = 0;
            stsIx     = 0;
            startStatus(ctrl);
            ack(dataIn, dataOut, ctrl);
            break;

        default:
            break;
    }

}

void
GenericSASIDrive::processData(BYTE& dataIn,
                              BYTE& dataOut,
                              BYTE& ctrl)
{
    off_t off;
    long  e;

    switch (cmdBuf[0])
    {
        case cmd_Write_c:
            off = ((((((cmdBuf[1] & 0x1f) << 8) | cmdBuf[2]) << 8) | cmdBuf[3]) +
                   blockCount) * driveSecLen;

            if (off >= capacity)
            {
                startError(ctrl, 0x21); // illegal disk address
                ack(dataIn, dataOut, ctrl);
                break;
            }

            lseek(driveFd, off + dataOffset, SEEK_SET);
            e = write(driveFd, dataBuf, dataLength);

            if (e != dataLength)
            {
                startError(ctrl, 0x94); // target sector not found + addr valid
                ack(dataIn, dataOut, ctrl);
                break;
            }

            if (++blockCount >= cmdBuf[4])
            {
                startStatus(ctrl);
                break;
            }

            break;

        case cmd_WriteSecBuf_c:
            // just leave sector buf with data for subsequent command(s)
            startStatus(ctrl);
            break;

        case cmd_WriteLong_c:
            break;

        default:
            break;
    }
}

void
GenericSASIDrive::processDCB(BYTE& dataIn,
                             BYTE& dataOut,
                             BYTE& ctrl)
{
    switch (cmdBuf[0])
    {
        case cmd_InitDriveChar_c:
        {
            // validate? update params?
            int cyl = (dcbBuf[0] << 8) | dcbBuf[1];
            int hds = dcbBuf[2] & 0x0f;
            int rwc = (dcbBuf[3] << 8) | dcbBuf[4];
            int wpc = (dcbBuf[5] << 8) | dcbBuf[6];
            int ebl = dcbBuf[7] & 0x0f;

            if (cyl != params[driveType][0] || hds != params[driveType][1] ||
                rwc != params[driveType][2] ||
                wpc != params[driveType][3])
            {
                debugss(ssMMS77320, ERROR, "Host drive characteristics "
                        " mismatch\n");
                // TODO: what to do. Host will be sending commands based
                // on a different geometry. Can we return an error?
                // startError(ctrl, 0x??);
            }

            startStatus(ctrl);
            ack(dataIn, dataOut, ctrl);
        }
        break;

        default:
            break;
    }
}

std::string
GenericSASIDrive::getMediaName()
{
    return (driveMedia != NULL ? driveMedia : "");
}
