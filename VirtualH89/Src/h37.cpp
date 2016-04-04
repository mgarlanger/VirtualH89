/// \file h37.cpp
///
/// \date May 13, 2009
/// \author Mark Garlanger
///

#include "h37.h"

#include "H89.h"
#include "logger.h"
#include "DiskDrive.h"
#include "wd1797.h"


Z_89_37::Z_89_37(int baseAddr): DiskController(baseAddr, H37_NumPorts_c),
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
    // wd1797 = new WD1797;

    drives_m[ds0] = nullptr;
    drives_m[ds1] = nullptr;
    drives_m[ds2] = nullptr;
    drives_m[ds3] = nullptr;
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
    dataEncoding_m      = FM;
    curDiskDrive_m      = numDisks_c;
    intLevel_m          = z_89_37_Intr_c;
    intrqAllowed_m      = false;
    drqAllowed_m        = false;
    cycleCount_m        = 0;

    motorOn_m           = false;
    dataReady_m         = false;
    lostDataStatus_m    = false;

    drives_m[ds0]       = nullptr;
    drives_m[ds1]       = nullptr;
    drives_m[ds2]       = nullptr;
    drives_m[ds3]       = nullptr;
}

BYTE
Z_89_37::in(BYTE addr)
{
    BYTE offset = getPortOffset(addr);
    BYTE val    = 0;

    debugss(ssH37, ALL, "H37::in(%d)\n", addr);

    switch (offset)
    {
        case ControlPort_Offset_c:
            debugss(ssH37, INFO, "H37::in(ControlPort)");
            val = controlReg_m;
            break;

        case InterfaceControl_Offset_c:
            debugss(ssH37, INFO, "H37::in(InterfaceControl)");

            if (sectorTrackAccess_m)
            {
                val = interfaceReg_m;
            }

            break;

        case StatusPort_Offset_c:
            if (sectorTrackAccess_m)
            {
                debugss(ssH37, INFO, "H37::in(SectorPort)");
                val = wd1797->in(SectorPort_Offset_c);
            }
            else
            {
                debugss(ssH37, INFO, "H37::in(StatusPort)");
                val = wd1797->in(StatusPort_Offset_c);
                // lowerIntrq();
            }

            break;

        case DataPort_Offset_c:
            if (sectorTrackAccess_m)
            {
                debugss(ssH37, INFO, "H37::in(TrackPort)");
                val = wd1797->in(TrackPort_Offset_c);
            }
            else
            {
                debugss(ssH37, INFO, "H37::in(DataPort)");
                val = wd1797->in(DataPort_Offset_c);

            }

            break;

        default:
            debugss(ssH37, ERROR, "H37::in(Unknown - 0x%02x)", addr);
            break;

    }

    debugss_nts(ssH37, INFO, " - %d(0x%x)\n", val, val);
    return (val);
}

void
Z_89_37::out(BYTE addr,
             BYTE val)
{
    BYTE offset = getPortOffset(addr);

    debugss(ssH37, ALL, "H37::out(%d, %d (0x%x))\n", addr, val, val);

    switch (offset)
    {
        case ControlPort_Offset_c:
            debugss(ssH37, INFO, "H37::out(ControlPort)");
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
            break;

        case InterfaceControl_Offset_c:
            debugss(ssH37, INFO, "H37::out(InterfaceControl) - ");

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
                debugss(ssH37, INFO, "H37::out(SectorPort): %d\n", val);

                wd1797->out(SectorPort_Offset_c, val);
            }
            else
            {
                debugss(ssH37, INFO, "H37::out(CommandPort): %d\n", val);
                wd1797->out(CommandPort_Offset_c, val);

            }

            break;

        case DataPort_Offset_c:
            if (sectorTrackAccess_m)
            {
                debugss(ssH37, INFO, "H37::out(TrackPort): %d\n", val);
                wd1797->out(TrackPort_Offset_c, val);
            }
            else
            {
                debugss(ssH37, INFO, "H37::out(DataPort): %d\n", val);
                wd1797->out(DataPort_Offset_c, val);
            }

            break;

        default:
            debugss(ssH37, ERROR, "H37::out(Unknown - 0x%02x): %d\n", addr, val);
            break;
    }
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
Z_89_37::removeDrive(BYTE unitNum)
{

    return (false);
}

void
Z_89_37::raiseIntrq()
{
    // check if IRQs are allowed.
    debugss(ssH37, INFO, "\n");

    if (intrqAllowed_m)
    {
        h89.raiseINT(z_89_37_Intr_c);
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
//      h89.raiseINT(z_89_37_Intr_c);
        h89.continueCPU();
        return;
    }

    debugss(ssH37, INFO, "not allowed.\n");

}

void
Z_89_37::lowerIntrq()
{
    debugss(ssH37, INFO, "\n");

    if (!intrqAllowed_m)
    {
        h89.lowerINT(z_89_37_Intr_c);
        return;
    }
}

void
Z_89_37::lowerDrq()
{
    debugss(ssH37, INFO, "\n");

    if (!drqAllowed_m)
    {
//        h89.lowerINT(z_89_37_Intr_c);
        return;
    }

}
