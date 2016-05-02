/// \file mms77316.cpp
///
/// Implementation of the MMS 77316 floppy disk controller.
///
/// \date Jan 30, 2016
/// \author Douglas Miller, cloned from h37.cpp by Mark Garlanger
///

#include <string.h>

#include "mms77316.h"

#include "logger.h"
#include "GenericFloppyDrive.h"
#include "RawFloppyImage.h"
#include "SectorFloppyImage.h"
#include "InterruptController.h"

const char* MMS77316::MMS77316_Name_c = "MMS77316";

GenericFloppyDrive*
MMS77316::getCurDrive()
{
    // This could be NULL
    return drives_m[controlReg_m & ctrl_DriveSel_c];
}

int
MMS77316::getClockPeriod()
{
    return ((controlReg_m & ctrl_525DriveSel_c) != 0 ? 1000 : 500);
}

MMS77316::MMS77316(int                  baseAddr,
                   InterruptController* ic): DiskController(baseAddr,
                                                            MMS77316_NumPorts_c),
                                             WD1797(baseAddr + Wd1797_Offset_c),
                                             ic_m(ic),
                                             controlReg_m(0),
                                             intLevel_m(MMS77316_Intr_c),
                                             drqCount_m(0)
{
    for (int x = 0; x < numDisks_c; ++x)
    {
        drives_m[x] = nullptr;
    }
}

std::vector<GenericDiskDrive*>
MMS77316::getDiskDrives()
{
    std::vector<GenericDiskDrive*> drives;

    for (int x = 0; x < numDisks_c; ++x)
    {
        drives.push_back(drives_m[x]); // might be null, but must preserve number.
    }

    return drives;
}

std::string
MMS77316::getDriveName(int index)
{
    if (index < 0 || index >= numDisks_c)
    {
        return NULL;
    }

    char        buf[32];
    sprintf(buf, "%s-%d", MMS77316_Name_c, index + 1);
    std::string str(buf);
    return str;
}

GenericDiskDrive*
MMS77316::findDrive(std::string ident)
{

    if (ident.find(MMS77316_Name_c) != 0 || ident[strlen(MMS77316_Name_c)] != '-')
    {
        return NULL;
    }

    char*         e;
    unsigned long x = strtoul(ident.c_str() + strlen(MMS77316_Name_c) + 1, &e, 10);

    if (*e != '\0' || x == 0 || x > numDisks_c)
    {
        return NULL;
    }

    return drives_m[x - 1];
}

MMS77316*
MMS77316::install_MMS77316(InterruptController*        ic,
                           PropertyUtil::PropertyMapT& props,
                           std::string                 slot)
{
    std::map<int, GenericFloppyDrive*> mmsdrives;
    std::string                        s;
    MMS77316*                          m316 = new MMS77316(BasePort_c, ic);

    // First identify what drives are installed.
    for (int x = 0; x < numDisks_c; ++x)
    {
        std::string prop = "mms77316_drive";
        prop += ('0' + x + 1);
        s     = props[prop];

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
        s     = props[prop];

        if (!s.empty())
        {
            GenericFloppyDrive* drv = m316->getDrive(x);

            if (drv != NULL)
            {
                drv->insertDisk(SectorFloppyImage::getDiskette(drv, PropertyUtil::splitArgs(s)));
            }
        }
    }

    return m316;
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

void
MMS77316::reset(void)
{
    controlReg_m  = 0;
    intrqRaised_m = false;
    drqRaised_m   = false;
    ic_m->setIntrq(intrqRaised_m);
    ic_m->setDrq(drqRaised_m);

    WD1797::reset();
}

BYTE
MMS77316::in(BYTE addr)
{
    BYTE offset = getPortOffset(addr);
    BYTE val    = 0;

    // case ControlPort_Offset_c: NOT READABLE
    if (offset >= Wd1797_Offset_c)
    {
        if (offset - Wd1797_Offset_c == DataPort_Offset_c)
        {
            // might need to simulate WAIT states...
            // Must NOT wait too long - this blocks all other progress.
            // TODO: redesign this to return to execute() loop and
            // stall there... requires in() to return a status or
            // some other way inform the CPU to stall. The MMS77316
            // has a built-in timeout on the WAIT hardware anyway,
            // so, insure we don't stay here forever. The hardware
            // timed out after 16 busclk (2MHz, i.e. CPU clock) cycles,
            // really should count those but this is probably close enough.
            int timeout = 0;

            while (burstMode() && !drqRaised_m && !intrqRaised_m && ++timeout < 16)
            {
                // TODO: this stalls
                waitForData();
            }
        }

        val = WD1797::in(addr);
    }
    else
    {
        debugss(ssMMS77316, ERROR, "(Unknown addr - 0x%02x)\n", addr);
    }

    debugss(ssMMS77316, VERBOSE, "(addr: %d) - %d\n", addr, val);
    return (val);
}

void
MMS77316::out(BYTE addr,
              BYTE val)
{
    BYTE offset = getPortOffset(addr);

    debugss(ssMMS77316, VERBOSE, "(addr: %d, %d (0x%x))\n", addr, val, val);

    if (offset >= Wd1797_Offset_c)
    {
        if (offset - Wd1797_Offset_c == DataPort_Offset_c)
        {
            // See notes for MMS77316::in()...
            int timeout = 0;

            while (burstMode() && !drqRaised_m && !intrqRaised_m && ++timeout < 16)
            {
                waitForData();
            }
        }

        WD1797::out(addr, val);
    }
    else if (offset == ControlPort_Offset_c)
    {
        debugss(ssMMS77316, VERBOSE, "(ControlPort) %02x\n", val);
        controlReg_m = val;
        drqCount_m   = 0;

        if ((controlReg_m & ctrl_525DriveSel_c) != 0)
        {
            GenericFloppyDrive* drive = getCurDrive();

            if (drive)
            {
                drive->motor(true);
            }
        }

        if ((controlReg_m & ctrl_EnableIntReq_c) != 0 && (intrqRaised_m || drqRaised_m))
        {
            // Interrupt still pending, but now un-masked. Raise it.
            ic_m->setIntrq(intrqRaised_m);
            ic_m->setDrq(drqRaised_m);
        }

    }
    else
    {
        debugss(ssMMS77316, ERROR, "(Unknown addr- 0x%02x): %d\n", addr, val);
    }
}

GenericFloppyDrive*
MMS77316::getDrive(BYTE unitNum)
{
    if (unitNum < numDisks_c)
    {
        return drives_m[unitNum];
    }

    return NULL;
}

bool
MMS77316::connectDrive(BYTE                unitNum,
                       GenericFloppyDrive* drive)
{
    bool retVal = false;

    debugss(ssMMS77316, INFO, "unit (%d), drive (%p)\n", unitNum, drive);

    if (unitNum < numDisks_c)
    {
        if (drives_m[unitNum] == NULL)
        {
            drives_m[unitNum] = drive;
            retVal            = true;
        }
        else
        {
            debugss(ssMMS77316, ERROR, "drive already connect\n");
        }
    }
    else
    {
        debugss(ssMMS77316, ERROR, "Invalid unit number (%d)\n", unitNum);
    }

    return (retVal);
}

bool
MMS77316::removeDrive(BYTE unitNum)
{
    return (false);
}

void
MMS77316::raiseIntrq()
{
    debugss(ssMMS77316, VERBOSE, "\n");

    drqCount_m    = 0;
    intrqRaised_m = true;

    if (intrqAllowed())
    {
        ic_m->setIntrq(intrqRaised_m);
    }
}

void
MMS77316::raiseDrq()
{
    debugss(ssMMS77316, INFO, "\n");

    drqRaised_m = true;

    if (drqAllowed())
    {
        ++drqCount_m;
        ic_m->setDrq(drqRaised_m);
    }
}

void
MMS77316::lowerIntrq()
{
    debugss(ssMMS77316, INFO, "\n");

    intrqRaised_m = false;
    ic_m->setIntrq(intrqRaised_m);
}

void
MMS77316::lowerDrq()
{
    debugss(ssMMS77316, INFO, "\n");

    drqRaised_m = false;
    ic_m->setDrq(drqRaised_m);
}

void
MMS77316::loadHead(bool load)
{
    debugss(ssMMS77316, INFO, "\n");
    WD1797::loadHead(load);

    if (!load)
    {
        // TODO: fix this... need timeout...
        // debugss(ssMMS77316, ERROR, "Clearing controlReg_m\n");
        // controlReg_m = 0;
        // TODO: start timer for motor-off (if was on...)
    }
}

std::string
MMS77316::dumpDebug()
{
    std::string ret = PropertyUtil::sprintf(
        "CTRL=%02x\n"
        "FDC-STS=%02x FDC-CMD=%02x\n"
        "FDC-TRK=%d FDC-SEC=%d FDC-DAT=%02x\n"
        "DRQ=%d INTRQ=%d\n",
        controlReg_m,
        statusReg_m, cmdReg_m,
        trackReg_m, sectorReg_m, dataReg_m,
        drqRaised_m, intrqRaised_m);
    return ret;
}
