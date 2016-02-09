/// \file mms77316.cpp
///
/// \date Jan 30, 2016
/// \author Douglas Miller, cloned from h37.cpp by Mark Garlanger
///

#include "H89.h"
#include "mms77316.h"
#include "logger.h"
#include "GenericFloppyDrive.h"
#include "RawFloppyImage.h"
#include <string.h>

const char *MMS77316::MMS77316_Name_c = "MMS77316";

GenericFloppyDrive *MMS77316::getCurDrive()
{
    // This could be NULL
    return drives_m[controlReg_m & ctrl_DriveSel_c];
}

int MMS77316::getClockPeriod()
{
    return ((controlReg_m & ctrl_525DriveSel_c) != 0 ? 1000 : 500);
}

MMS77316::MMS77316(int baseAddr):
    DiskController(baseAddr, MMS77316_NumPorts_c),
    WD1797(baseAddr + Wd1797_Offset_c),
    controlReg_m(0),
    intLevel_m(MMS77316_Intr_c)
{
    for (int x = 0; x < numDisks_c; ++x)
    {
        drives_m[x] = nullptr;
    }

    h89.registerInter(interResponder, this);
}

std::vector<GenericDiskDrive *> MMS77316::getDiskDrives()
{
    std::vector<GenericDiskDrive *> drives;

    for (int x = 0; x < numDisks_c; ++x)
    {
        drives.push_back(drives_m[x]); // might be null, but must preserve number.
    }

    return drives;
}

std::string MMS77316::getDriveName(int index)
{
    if (index < 0 || index >= numDisks_c)
    {
        return NULL;
    }

    char buf[32];
    sprintf(buf, "%s-%d", MMS77316_Name_c, index + 1);
    std::string str(buf);
    return str;
}

GenericDiskDrive *MMS77316::findDrive(std::string ident)
{

    if (ident.find(MMS77316_Name_c) != 0 || ident[strlen(MMS77316_Name_c)] != '-')
    {
        return NULL;
    }

    char *e;
    int x = strtoul(ident.c_str() + strlen(MMS77316_Name_c) + 1, &e, 10);

    if (*e != '\0' || x == 0 || x > numDisks_c)
    {
        return NULL;
    }

    return drives_m[x - 1];
}

MMS77316 *MMS77316::install_MMS77316(PropertyUtil::PropertyMapT& props, std::string slot)
{
    std::map<int, GenericFloppyDrive *> mmsdrives;
    std::string s;
    MMS77316 *m316 = new MMS77316(BasePort_c);

    // First identify what drives are installed.
    for (int x = 0; x < numDisks_c; ++x)
    {
        std::string prop = "mms77316_drive";
        prop += ('0' + x + 1);
        s = props[prop];

        if (!s.empty())
        {
            mmsdrives[x] = GenericFloppyDrive::getInstance(s);
        }
    }

    // Now connect drives to controller.
    if (mmsdrives.size() > 0)
    {

        for (int x = 0; x < numDisks_c; ++x)
        {
            if (mmsdrives[x] != NULL)
            {
                m316->connectDrive(x, mmsdrives[x]);
            }
        }
    }

    // Next identify what diskettes are pre-inserted.
    // TODO: GUI should allow manipulation of diskettes.
    for (int x = 0; x < numDisks_c; ++x)
    {
        std::string prop = "mms77316_disk";
        prop += ('0' + x + 1);
        s = props[prop];

        if (!s.empty())
        {
            GenericFloppyDrive *drv = m316->getDrive(x);

            if (drv != NULL)
            {
                drv->insertDisk(new RawFloppyImage(drv, PropertyUtil::splitArgs(s)));
            }
        }
    }

    return m316;
}

unsigned long MMS77316::interResponder(void *data, int level)
{
    MMS77316 *thus = (MMS77316 *)data;
    unsigned long intr = CPU::NO_INTR_INST;

    if (level == MMS77316_Intr_c && thus->intrqAllowed())
    {
        // TODO: generate op-codes without knowing Z80CPU?
        intr = (thus->intrqRaised_m ? 0xf7 : 0xfb);
    }

    thus->intrqRaised_m = false;
    return intr;
}

MMS77316::~MMS77316()
{
    for (int x = 0; x < numDisks_c; ++x)
    {
        if (drives_m[x] != NULL)
        {
            drives_m[x]->insertDisk(NULL);
        }
    }
}

void MMS77316::reset(void)
{
    WD1797::reset();
    // TBD: Is this "system reset" (RSTN)? orr something else?
    // because system reset should not disconnect drives...
    controlReg_m        = 0;
    intLevel_m          = MMS77316_Intr_c;

    for (int x = 0; x < numDisks_c; ++x)
    {
        drives_m[x] = nullptr;
    }
}

BYTE MMS77316::in(BYTE addr)
{
    BYTE offset = getPortOffset(addr);
    BYTE val = 0;

    debugss(ssMMS77316, ALL, "MMS77316::in(%d)\n", addr);

    // case ControlPort_Offset_c: NOT READABLE
    if (offset >= Wd1797_Offset_c)
    {
        if (offset - Wd1797_Offset_c == DataPort_Offset_c)
        {
            // might need to simulate WAIT states...
            while (burstMode() && !drqRaised_m && !intrqRaised_m)
            {
                waitForData();
            }
        }

        val = WD1797::in(addr);
    }

    else
    {
        debugss(ssMMS77316, ERROR, "MMS77316::in(Unknown - 0x%02x)\n", addr);
    }

    debugss_nts(ssMMS77316, INFO, " - %d(0x%x)\n", val, val);
    return (val);
}

void MMS77316::out(BYTE addr, BYTE val)
{
    BYTE offset = getPortOffset(addr);

    debugss(ssMMS77316, ALL, "MMS77316::out(%d, %d (0x%x))\n", addr, val, val);

    if (offset >= Wd1797_Offset_c)
    {
        if (offset - Wd1797_Offset_c == DataPort_Offset_c)
        {
            // might need to simulate WAIT states...
            while (burstMode() && !drqRaised_m && !intrqRaised_m)
            {
                waitForData();
            }
        }

        WD1797::out(addr, val);
    }

    else if (offset == ControlPort_Offset_c)
    {
        debugss(ssMMS77316, INFO, "MMS77316::out(ControlPort) %02x\n", val);
        controlReg_m = val;

        if ((controlReg_m & ctrl_525DriveSel_c) != 0)
        {
            GenericFloppyDrive *drive = getCurDrive();

            if (drive)
            {
                drive->motor(true);
            }
        }

        debugss_nts(ssMMS77316, INFO, "\n");
    }

    else
    {
        debugss(ssMMS77316, ERROR, "MMS77316::out(Unknown - 0x%02x): %d\n", addr, val);
    }
}

GenericFloppyDrive *MMS77316::getDrive(BYTE unitNum)
{
    if (unitNum < numDisks_c)
    {
        return drives_m[unitNum];
    }

    return NULL;
}

bool MMS77316::connectDrive(BYTE unitNum, GenericFloppyDrive *drive)
{
    bool retVal = false;

    debugss(ssMMS77316, INFO, "%s: unit (%d), drive (%p)\n", __FUNCTION__, unitNum, drive);

    if (unitNum < numDisks_c)
    {
        if (drives_m[unitNum] == NULL)
        {
            drives_m[unitNum] = drive;
            retVal = true;
        }

        else
        {
            debugss(ssMMS77316, ERROR, "%s: drive already connect\n", __FUNCTION__);
        }
    }

    else
    {
        debugss(ssMMS77316, ERROR, "%s: Invalid unit number (%d)\n", __FUNCTION__, unitNum);
    }

    return (retVal);
}

bool MMS77316::removeDrive(BYTE unitNum)
{
    return (false);
}

void MMS77316::raiseIntrq()
{
    debugss(ssMMS77316, INFO, "%s\n", __FUNCTION__);

    if (intrqAllowed())
    {
        h89.raiseINT(MMS77316_Intr_c);
        WD1797::raiseIntrq();
        h89.continueCPU();
        return;
    }
}

void MMS77316::raiseDrq()
{
    debugss(ssMMS77316, INFO, "%s\n", __FUNCTION__);

    if (intrqAllowed() && !burstMode())
    {
        h89.raiseINT(MMS77316_Intr_c);
        WD1797::raiseDrq();
        h89.continueCPU();
    }
}

void MMS77316::lowerIntrq()
{
    debugss(ssMMS77316, INFO, "%s\n", __FUNCTION__);
    WD1797::lowerIntrq();
    // TODO: only lower if !drqRaised_m, but Z80 is not handling that correctly anyway.
    h89.lowerINT(MMS77316_Intr_c);
}

void MMS77316::lowerDrq()
{
    debugss(ssMMS77316, INFO, "%s\n", __FUNCTION__);
    WD1797::lowerDrq();
    // TODO: only lower if !intrqRaised_m, but Z80 is not handling that correctly anyway.
    h89.lowerINT(MMS77316_Intr_c);
}

void MMS77316::loadHead(bool load)
{
    debugss(ssMMS77316, INFO, "%s\n", __FUNCTION__);
    WD1797::loadHead(load);

    if (!load)
    {
        // TODO: fix this... need timeout...
        //debugss(ssMMS77316, ERROR, "Clearing controlReg_m\n");
        //controlReg_m = 0;
        // TODO: start timer for motor-off (if was on...)
    }
}
