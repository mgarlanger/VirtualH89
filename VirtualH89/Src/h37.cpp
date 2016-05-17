/// \file h37.cpp
///
/// \date May 13, 2009
/// \author Mark Garlanger
///

#include "h37.h"

#include "logger.h"
#include "DiskDrive.h"
#include "wd1797.h"
#include "SoftSectoredDisk.h"
#include "InterruptController.h"
#include "GenericFloppyDrive.h"
#include "computer.h"
#include "GenericFloppyDisk.h"


Z_89_37::Z_89_37(Computer*            computer,
                 int                  baseAddr,
                 InterruptController* ic): DiskController(baseAddr,
                                                          H37_NumPorts_c),
                                           computer_m(computer),
                                           ic_m(ic),
                                           interfaceReg_m(0),
                                           controlReg_m(0),
                                           motorOn_m(false),
                                           dataReady_m(false),
                                           lostDataStatus_m(false),
                                           sectorTrackAccess_m(false),
                                           dataEncoding_m(FM),
                                           curDiskDrive_m(numDisks_c),
                                           intLevel_m(z_89_37_Intr_c),
                                           curPos_m(0),
                                           sectorPos_m(-10),
                                           intrqAllowed_m(false),
                                           drqAllowed_m(false),
                                           cycleCount_m(0)
{
    wd1797               = new WD1797(this);

    drives_m[ds0]        = nullptr;
    drives_m[ds1]        = nullptr;
    drives_m[ds2]        = nullptr;
    drives_m[ds3]        = nullptr;
    genericDrives_m[ds0] = nullptr;
    genericDrives_m[ds1] = nullptr;
    genericDrives_m[ds2] = nullptr;
    genericDrives_m[ds3] = nullptr;

}

Z_89_37::~Z_89_37()
{

}

void
Z_89_37::reset(void)
{
    interfaceReg_m      = 0;
    controlReg_m        = 0;
    sectorTrackAccess_m = false;
    intrqAllowed_m      = false;
    drqAllowed_m        = false;
    motorOn_m           = false;
    dataReady_m         = false;
    lostDataStatus_m    = false;
    wd1797->reset();
}


Z_89_37*
Z_89_37::install_H37(Computer*                   computer,
                     BYTE                        baseAddr,
                     InterruptController*        ic,
                     PropertyUtil::PropertyMapT& props,
                     std::string                 slot)
{
    std::string                   s;
    Z_89_37*                      z37 = new Z_89_37(computer, baseAddr, ic);

    debugss(ssH37, INFO, "entering\n");

    for (BYTE i = 0; i < numDisks_c; ++i)
    {

        std::string prop = "h37_drive";
        prop += ('0' + i + 1);
        s     = props[prop];

        if (!s.empty())
        {
#if 1
            GenericFloppyDrive* drive = GenericFloppyDrive::getInstance(s);
            if (drive)
            {

                z37->connectDrive(i, drive);

                prop  = "h37_disk";
                prop += ('0' + i + 1);
                s     = props[prop];

                if (!s.empty())
                {
                    /*SoftSectoredDisk* disk = new SoftSectoredDisk(
                                                                  s.c_str(), SoftSectoredDisk::dif_RAW);
                       drive->insertDisk(disk);*/
                    drive->insertDisk(GenericFloppyDisk::loadDiskImage(PropertyUtil::splitArgs(s)));
                }

            }

#else
            DiskDrive* drive = DiskDrive::getInstance(s);
            if (drive)
            {

                z37->connectDrive(i, drive);
                prop  = "z37_disk";
                prop += ('0' + i + 1);
                s     = props[prop];

                s     = props["z37_disk1"];

                if (!s.empty())
                {
                    SoftSectoredDisk* disk = new SoftSectoredDisk(
                        s.c_str(), SoftSectoredDisk::dif_RAW);
                    drive->insertDisk(disk);
                }

            }
#endif
        }
    }

    return z37;
}

BYTE
Z_89_37::in(BYTE addr)
{
    BYTE offset = getPortOffset(addr);
    BYTE val    = 0;

    debugss(ssH37, ALL, "(%d)\n", addr);

    switch (offset)
    {
        case ControlPort_Offset_c:
            debugss(ssH37, INFO, "(ControlPort) - %d\n", controlReg_m);
            val = controlReg_m;
            break;

        case InterfaceControl_Offset_c:
            debugss(ssH37, INFO, "(InterfaceControl) - %d\n", sectorTrackAccess_m);

            if (sectorTrackAccess_m)
            {
                val = interfaceReg_m;
            }

            break;

        case StatusPort_Offset_c:
            if (sectorTrackAccess_m)
            {
                debugss(ssH37, INFO, "(SectorPort)\n");
                val = wd1797->in(WD1797::SectorPort_Offset_c);
            }
            else
            {
                debugss(ssH37, INFO, "(StatusPort)\n");
                val = wd1797->in(WD1797::StatusPort_Offset_c);
            }

            break;

        case DataPort_Offset_c:
            if (sectorTrackAccess_m)
            {
                debugss(ssH37, INFO, "(TrackPort)\n");
                val = wd1797->in(WD1797::TrackPort_Offset_c);
            }
            else
            {
                debugss(ssH37, INFO, "(DataPort)\n");
                val = wd1797->in(WD1797::DataPort_Offset_c);

            }

            break;

        default:
            debugss(ssH37, ERROR, "(Unknown - 0x%02x)", addr);
            break;

    }

    return (val);
}

void
Z_89_37::out(BYTE addr,
             BYTE val)
{
    debugss(ssH37, ALL, "(%d, %d (0x%x))\n", addr, val, val);

    BYTE offset = getPortOffset(addr);

    switch (offset)
    {
        case ControlPort_Offset_c:
            debugss(ssH37, INFO, "(ControlPort)");
            controlReg_m = val;

            if (val & ctrl_EnableIntReq_c)
            {
                debugss_nts(ssH37, INFO, " EnableIntReq");
                intrqAllowed_m = true;
            }
            else
            {
                debugss_nts(ssH37, INFO, " DisableIntReq");
                intrqAllowed_m = false;
            }

            if (val & ctrl_EnableDrqInt_c)
            {
                debugss_nts(ssH37, INFO, " EnableDrqInt");
                drqAllowed_m = true;
            }
            else
            {
                debugss_nts(ssH37, INFO, " DisableDrqReq");
                drqAllowed_m = false;
            }

            if (val & ctrl_SetMFMRecording_c)
            {
                debugss_nts(ssH37, INFO, " SetMFM");
                dataEncoding_m = MFM;
            }
            else
            {
                dataEncoding_m = FM;
            }

            motorOn(val & ctrl_MotorsOn_c);
            if (val & ctrl_MotorsOn_c)
            {
                debugss_nts(ssH37, INFO, " MotorsOn");
                motorOn_m = true;
            }
            else
            {
                motorOn_m = false;
            }

            if (val & ctrl_Drive_0_c)
            {
                debugss_nts(ssH37, INFO, " Drive0");
                curDiskDrive_m = ds0;
            }

            if (val & ctrl_Drive_1_c)
            {
                debugss_nts(ssH37, INFO, " Drive1");
                curDiskDrive_m = ds1;
            }

            if (val & ctrl_Drive_2_c)
            {
                debugss_nts(ssH37, INFO, " Drive2");
                curDiskDrive_m = ds2;
            }

            if (val & ctrl_Drive_3_c)
            {
                debugss_nts(ssH37, INFO, " Drive3");
                curDiskDrive_m = ds3;
            }

            debugss_nts(ssH37, INFO, "\n");

            // this APPEARS to be how the controller handles this.
            ic_m->blockInterrupts(drqAllowed_m);

            break;

        case InterfaceControl_Offset_c:
            debugss(ssH37, INFO, "(InterfaceControl) - ");

            interfaceReg_m = val;

            if ((val & if_SelectSectorTrack_c) == if_SelectSectorTrack_c)
            {
                debugss_nts(ssH37, INFO, "Sector/Track Access\n");
                sectorTrackAccess_m = true;
            }
            else
            {
                debugss_nts(ssH37, INFO, "Control/Data Access\n");
                sectorTrackAccess_m = false;
            }

            break;

        case CommandPort_Offset_c:
            if (sectorTrackAccess_m)
            {
                debugss(ssH37, INFO, "(SectorPort): %d\n", val);

                wd1797->out(WD1797::SectorPort_Offset_c, val);
            }
            else
            {
                debugss(ssH37, INFO, "(CommandPort): %d\n", val);
                wd1797->out(WD1797::CommandPort_Offset_c, val);

            }

            break;

        case DataPort_Offset_c:
            if (sectorTrackAccess_m)
            {
                debugss(ssH37, INFO, "(TrackPort): %d\n", val);
                wd1797->out(WD1797::TrackPort_Offset_c, val);
            }
            else
            {
                debugss(ssH37, INFO, "(DataPort): %d\n", val);
                wd1797->out(WD1797::DataPort_Offset_c, val);
            }

            break;

        default:
            debugss(ssH37, ERROR, "(Unknown - 0x%02x): %d\n", addr, val);
            break;
    }
}

void
Z_89_37::motorOn(bool motor)
{
    if (motor != motorOn_m)
    {
        motorOn_m = motor;
        for (int drive = ds0; drive < numDisks_c; drive++)
        {
            if (genericDrives_m[drive])
            {
                genericDrives_m[drive]->motor(motorOn_m);
            }
        }
    }
}


GenericFloppyDrive*
Z_89_37::getCurrentDrive()
{
    if (curDiskDrive_m < numDisks_c)
    {
        return genericDrives_m[curDiskDrive_m];
    }
    return nullptr;
}


bool
Z_89_37::connectDrive(BYTE       unitNum,
                      DiskDrive* drive)
{
    bool retVal = false;

    debugss(ssH37, INFO, "unit (%d), drive (%p)\n", unitNum, drive);

    if (unitNum < numDisks_c)
    {
        if (drives_m[unitNum] == 0)
        {
            drives_m[unitNum] = drive;
            retVal            = true;
        }
        else
        {
            debugss(ssH37, ERROR, "drive already connect\n");
        }
    }
    else
    {
        debugss(ssH37, ERROR, "Invalid unit number (%d)\n", unitNum);
    }

    return (retVal);
}

bool
Z_89_37::connectDrive(BYTE                unitNum,
                      GenericFloppyDrive* drive)
{
    bool retVal = false;

    debugss(ssH37, INFO, "unit (%d), drive (%p)\n", unitNum, drive);

    if (unitNum < numDisks_c)
    {
        if (genericDrives_m[unitNum] == 0)
        {
            genericDrives_m[unitNum] = drive;
            retVal                   = true;
        }
        else
        {
            debugss(ssH37, ERROR, "drive already connect\n");
        }
    }
    else
    {
        debugss(ssH37, ERROR, "Invalid unit number (%d)\n", unitNum);
    }

    return (retVal);
}

bool
Z_89_37::removeDrive(BYTE unitNum)
{
    // \todo implement
    return (false);
}

void
Z_89_37::raiseIntrq()
{
    // check if IRQs are allowed.
    debugss(ssH37, INFO, "\n");

    if (intrqAllowed_m)
    {
        ic_m->setIntrq(true);
        computer_m->continueCPU();
        return;
    }
}

void
Z_89_37::raiseDrq()
{
    debugss(ssH37, INFO, "\n");

    // check if DRQ is allowed.
    if (drqAllowed_m)
    {
        ic_m->setDrq(true);
        computer_m->continueCPU();
        return;
    }

    debugss(ssH37, INFO, "not allowed.\n");

}

void
Z_89_37::lowerIntrq()
{
    debugss(ssH37, INFO, "\n");

    // \todo - if intrqAllowed_m is disabled, should THAT trigger the
    // setting intrq to false.
    if (1) // (!intrqAllowed_m)
    {
        ic_m->setIntrq(false);
    }
}

void
Z_89_37::lowerDrq()
{
    debugss(ssH37, INFO, "\n");

    // \todo - if drqAllowed_m is disabled, should THAT trigger the
    // setting drq to false.
    if (1) // (!drqAllowed_m)
    {
        ic_m->setDrq(false);
    }

}

bool
Z_89_37::doubleDensity()
{
    debugss(ssH37, ALL, "\n");

    return dataEncoding_m == MFM;
}

bool
Z_89_37::readReady()
{
    // the H-37 has this line set to +5V.
    return true;
}

int
Z_89_37::getClockPeriod()
{
    return 1000;
}
