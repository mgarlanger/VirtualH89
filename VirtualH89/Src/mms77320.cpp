/// \file mms77320.cpp
///
/// Implementation of the MMS 77320 SASI adapter.
///
/// \date Feb 13, 2016
/// \author Douglas Miller
///

#include <string.h>

#include "mms77320.h"

#include "logger.h"
#include "H89.h"
#include "GenericSASIDrive.h"


const char* MMS77320::MMS77320_Name_c = "MMS77320";

GenericSASIDrive*
MMS77320::getCurDrive()
{
    // This could be NULL
    return drives_m[control1Reg_m & ctrl_DriveSel_c];
}

MMS77320::MMS77320(int baseAddr,
                   int intLevel,
                   int switches): DiskController(baseAddr, MMS77320_NumPorts_c),
                                  dataOutReg_m(0),
                                  dataInReg_m(0),
                                  control0Reg_m(0),
                                  control1Reg_m(-1),
                                  statusReg_m(0),
                                  switchReg_m(switches),
                                  ctrlBus_m(0),
                                  curDrive_m(NULL),
                                  intLevel_m(intLevel)
{
    for (int x = 0; x < numDisks_c; ++x)
    {
        drives_m[x] = nullptr;
    }
}

std::vector<GenericDiskDrive*> MMS77320::getDiskDrives()
{
    // No removable drives (?) so nothing returned here...
    std::vector<GenericDiskDrive*> drives;
    // TODO: handle removeable drives, and return those here...
#if 0

    for (int x = 0; x < numDisks_c; ++x)
    {
        drives.push_back(drives_m[x]); // might be null, but must preserve number.
    }

#endif
    return drives;
}

std::string
MMS77320::getDriveName(int index)
{
    if (index < 0 || index >= numDisks_c)
    {
        return NULL;
    }

    char        buf[32];
    sprintf(buf, "%s-%d", MMS77320_Name_c, index + 1);
    std::string str(buf);
    return str;
}

GenericDiskDrive*
MMS77320::findDrive(std::string ident)
{

    if (ident.find(MMS77320_Name_c) != 0 || ident[strlen(MMS77320_Name_c)] != '-')
    {
        return NULL;
    }

    char*         e;
    unsigned long x = strtoul(ident.c_str() + strlen(MMS77320_Name_c) + 1, &e, 10);

    if (*e != '\0' || x == 0 || x > numDisks_c)
    {
        return NULL;
    }

    return drives_m[x - 1];
}

MMS77320*
MMS77320::install_MMS77320(PropertyUtil::PropertyMapT& props,
                           std::string                 slot)
{
    std::map<int, GenericSASIDrive*> mmsdrives;
    std::string                      s;
    int                              port = 0x78;
    unsigned int                     sw   = 0;
    int                              intr = 0;
    std::string                      dir  = "";

    // TODO: just look at sw501 and make it match?
    // BYTE sw = h89.getIO().in(0xf2);
    // if ((sw & 0x03) == 0x02) port = 0x7c;
    // else if ((sw & 0x0c) == 0x08) port = 0x78;
    // Technically, the user must both strap the card and set SW501 appropriately.
    s = props["mms77320_port"];

    if (s.compare("jp1a") == 0)
    {
        port &= ~0x04;
    }
    else if (s.compare("jp1b") == 0)
    {
        port |= 0x04;
    }

    s = props["mms77320_intr"];

    if (s.compare("jp2a") == 0)
    {
        intr = 3;
    }
    else if (s.compare("jp2b") == 0)
    {
        intr = 4;
    }
    else if (s.compare("jp2c") == 0)
    {
        intr = 5;
    }

    s = props["mms77320_dipsw"];

    if (!s.empty())
    {
        sw = (unsigned int) strtoul(s.c_str(), NULL, 2);
    }

    dir = props["mms77320_dir"];

    MMS77320* m320 = new MMS77320(port, intr, sw);

    // First identify what drives are installed.
    for (int x = 0; x < numDisks_c; ++x)
    {
        std::string prop  = "mms77320_drive";
        std::string media = dir + "/" + MMS77320_Name_c + "-";
        media += ('0' + x);
        prop  += ('0' + x);
        s      = props[prop];

        if (!s.empty())
        {
            // TODO: handle removable media case...
            m320->connectDrive(x, GenericSASIDrive::getInstance(s, media, x));
        }
    }

    debugss(ssMMS77320, ERROR, "MMS77320 at port %02x INT%d\n", port, intr);

    return m320;
}

MMS77320::~MMS77320()
{
    for (int x = 0; x < numDisks_c; ++x)
    {
        if (drives_m[x] != NULL)
        {
            delete drives_m[x];
            drives_m[x] = NULL;
        }
    }
}

void
MMS77320::reset(void)
{
    dataOutReg_m  = 0;
    dataInReg_m   = 0;
    control0Reg_m = 0;
    control1Reg_m = -1;
    statusReg_m   = 0;
    ctrlBus_m     = 0;
    curDrive_m = NULL;
    h89.lowerINT(intLevel_m);
    // TODO: reset all drives?
}

BYTE
MMS77320::in(BYTE addr)
{
    BYTE offset = getPortOffset(addr);
    BYTE val    = 0;

    debugss(ssMMS77320, ALL, "MMS77320::in(%d)\n", addr);

    if (offset == DataPort_Offset_c)
    {
        val = dataInReg_m;
        debugss(ssMMS77320, INFO, "MMS77320::in(DataPort) %02x\n", val);

        if (curDrive_m != NULL)
        {
            ctrlBus_m |= GenericSASIDrive::ctl_Ack_i_c;
            curDrive_m->ack(dataInReg_m, dataOutReg_m, ctrlBus_m);
        }

        // TODO: side-effects?
    }
    else if (offset == StatusPort_Offset_c)
    {
        val = getStatus(ctrlBus_m);
        debugss(ssMMS77320, INFO, "MMS77320::in(StatusPort) %02x %s %s %s %s %s\n", val,
                (val & sts_Msg_c) ? "MSG" : "   ",
                (val & sts_Req_c) ? "REQ" : "   ",
                (val & sts_Cmd_c) ? "CMD" : "   ",
                (val & sts_POut_c) ? "OUT" : "   ",
                (val & sts_Busy_c) ? "BSY" : "   "
                );
        lowerIntrq();
        // TODO; other side-effects?
    }
    else
    {
        debugss(ssMMS77320, ERROR, "MMS77320::in(Unknown - 0x%02x)\n", addr);
    }

    debugss_nts(ssMMS77320, INFO, " - %d(0x%x)\n", val, val);
    return (val);
}

BYTE
MMS77320::getStatus(BYTE ctl)
{
    BYTE status = 0;

    if (ctl & GenericSASIDrive::ctl_Req_o_c)
    {
        status |= sts_Req_c;
    }

    if (ctl & GenericSASIDrive::ctl_Out_o_c)
    {
        status |= sts_POut_c;
    }

    if (ctl & GenericSASIDrive::ctl_Msg_o_c)
    {
        status |= sts_Msg_c;
    }

    if (ctl & GenericSASIDrive::ctl_Cmd_o_c)
    {
        status |= sts_Cmd_c;
    }

    if (ctl & GenericSASIDrive::ctl_Busy_o_c)
    {
        status |= sts_Busy_c;
    }

    if (ctl & GenericSASIDrive::ctl_Ack_i_c)
    {
        status |= sts_Ack_c;
    }

    // debugss(ssMMS77320, INFO, "raw getStatus %02x (ctl: %02x)\n", status, ctl);
    // TODO: get ACTINT bit...
    // return ~status; // or (status ^ 0xf8)...
    return status;
}

void
MMS77320::out(BYTE addr,
              BYTE val)
{
    BYTE offset = getPortOffset(addr);

    debugss(ssMMS77320, ALL, "MMS77320::out(%d, %d (0x%x))\n", addr, val, val);

    if (offset == DataPort_Offset_c)
    {
        debugss(ssMMS77320, INFO, "MMS77320::out(DataPort) %02x\n", val);
        dataOutReg_m = val;

        if (curDrive_m != NULL)
        {
            ctrlBus_m |= GenericSASIDrive::ctl_Ack_i_c;
            curDrive_m->ack(dataInReg_m, dataOutReg_m, ctrlBus_m);
        }

    }
    else if (offset == Control1Port_Offset_c)
    {
        val          &= 0x0f;
        debugss(ssMMS77320, INFO, "MMS77320::out(Control1Port) %02x\n", val);
        bool diff = ((control1Reg_m ^ val) & ctrl_DriveSel_c) != 0;
        control1Reg_m = val;

        if (diff)
        {
            if (curDrive_m != NULL)
            {
                // anything to do to de-select drive?
                curDrive_m->deselect(dataInReg_m, dataOutReg_m, ctrlBus_m);
            }

            curDrive_m = drives_m[control1Reg_m & ctrl_DriveSel_c];
            // anything else to do to select new drive?
            // actual SELECT is done later, on Control0Port_Offset_c...
        }

    }
    else if (offset == Control0Port_Offset_c)
    {
        val &= 0xf0;
        int diffs = (control0Reg_m ^ val);
        debugss(ssMMS77320, INFO, "MMS77320::out(Control0Port) %02x [%02x] (%s)\n", val, diffs,
                curDrive_m != NULL ? "OK" : "null");
        control0Reg_m = val;

        if (diffs && curDrive_m != NULL)
        {
            if (control0Reg_m == 0)
            {
                ctrlBus_m &= ~GenericSASIDrive::ctl_Sel_i_c;
                ctrlBus_m &= ~GenericSASIDrive::ctl_Reset_i_c;
                curDrive_m->run(dataInReg_m, dataOutReg_m, ctrlBus_m);
            }
            else if (control0Reg_m & ctrl_ResetStart_c)
            {
                debugss(ssMMS77320, INFO, "RESET SASI\n");
                ctrlBus_m |= GenericSASIDrive::ctl_Reset_i_c;
                curDrive_m->resetSASI(dataInReg_m, dataOutReg_m, ctrlBus_m);
            }
            else if (control0Reg_m & ctrl_Select_c)
            {
                BYTE temp = (1 << (control1Reg_m & ctrl_DriveSel_c));
                ctrlBus_m |= GenericSASIDrive::ctl_Sel_i_c;
                curDrive_m->select(dataInReg_m, temp, ctrlBus_m);
            }
        }
    }
    else
    {
        debugss(ssMMS77320, ERROR, "MMS77320::out(Unknown - 0x%02x): %d\n", addr, val);
    }
}

GenericSASIDrive*
MMS77320::getDrive(BYTE unitNum)
{
    if (unitNum < numDisks_c)
    {
        return drives_m[unitNum];
    }

    return NULL;
}

bool
MMS77320::connectDrive(BYTE              unitNum,
                       GenericSASIDrive* drive)
{
    bool retVal = false;

    debugss(ssMMS77320, INFO, "%s: unit (%d), drive (%p)\n", __FUNCTION__, unitNum, drive);

    if (unitNum < numDisks_c)
    {
        if (drives_m[unitNum] == NULL)
        {
            drives_m[unitNum] = drive;
            retVal            = true;
        }
        else
        {
            debugss(ssMMS77320, ERROR, "%s: drive already connect\n", __FUNCTION__);
        }
    }
    else
    {
        debugss(ssMMS77320, ERROR, "%s: Invalid unit number (%d)\n", __FUNCTION__, unitNum);
    }

    return (retVal);
}

bool
MMS77320::removeDrive(BYTE unitNum)
{
    return (false);
}

void
MMS77320::raiseIntrq()
{
    debugss(ssMMS77320, INFO, "%s\n", __FUNCTION__);

    if (intrqAllowed())
    {
        h89.raiseINT(intLevel_m);
        h89.continueCPU();
        return;
    }
}

void
MMS77320::lowerIntrq()
{
    debugss(ssMMS77320, INFO, "%s\n", __FUNCTION__);
    h89.lowerINT(intLevel_m);
}

std::string
MMS77320::dumpDebug()
{
    std::string ret = PropertyUtil::sprintf(
        "CTRL-0=%02x     CTRL-1=%02x\n"
        "DAT-IN=%02x    DAT-OUT=%02x\n"
        "STATUS=%02x\n",
        control0Reg_m, control1Reg_m,
        dataInReg_m, dataOutReg_m,
        getStatus(ctrlBus_m));
    return ret;
}
