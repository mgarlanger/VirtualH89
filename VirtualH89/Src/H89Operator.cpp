/// \file H89Operator.cpp
///
/// The VirtualH89-side implementation of the H89 Operator's interface, for
/// standard functions such as RESET and diskette muont/unmount, but also
/// providing virtual features like debugging.
///
/// \date Feb 9, 2016
/// \author Douglas Miller
///

#include "H89Operator.h"


#include "cpu.h"
#include "H89.h"
#include "h89-io.h"
#include "logger.h"
#include "DiskController.h"
#include "GenericDiskDrive.h"
#include "RawFloppyImage.h"
#include "SectorFloppyImage.h"
#include "propertyutil.h"

/// \cond
#include <stdio.h>
#include <unistd.h>
#include <pthread.h>
#include <signal.h>
#include <string.h>
#include <termios.h>
#include <unistd.h>
#include <sstream>
/// \endcond


/// \brief H89Operator
///
///
H89Operator::H89Operator()
{
}

H89Operator::~H89Operator() {
}

GenericDiskDrive*
H89Operator::findDrive(std::string name)
{
    std::vector<DiskController*> devs = h89.getIO().getDiskDevices();

    for (int x = 0; x < devs.size(); ++x)
    {
        DiskController*   dev = devs[x];
        GenericDiskDrive* drv = nullptr;

        if (dev != nullptr && (drv = dev->findDrive(name)) != nullptr)
        {
            return drv;
        }
    }

    return nullptr;
}

DiskController*
H89Operator::findDiskCtrlr(std::string name)
{
    std::vector<DiskController*> devs = h89.getIO().getDiskDevices();

    for (int x = 0; x < devs.size(); ++x)
    {
        DiskController* dev = devs[x];

        if (dev != nullptr && name.compare(dev->getDeviceName()) == 0)
        {
            return dev;
        }
    }

    return nullptr;
}

std::string
H89Operator::cleanse(std::string resp)
{
    size_t pos = 0;

    while ((pos = resp.find('\n', pos)) != std::string::npos)
    {
        resp.replace(pos, 1, ";");
        pos += 1;
    }

    return resp;
}

std::string
H89Operator::executeCommand(std::string cmd)
{
    std::string              resp;
    std::vector<std::string> args = PropertyUtil::splitArgs(cmd);
    debugss(ssStdioConsole, INFO, "Servicing cmd = %s\n", cmd.c_str());

    if (args.size() < 1)
    {
        return "ok";
    }

    if (args[0].compare("quit") == 0)
    {
        h89.systemMutexRelease();
        exit(0);
    }

    if (args[0].compare("echo") == 0)
    {
        return "ok " + PropertyUtil::combineArgs(args, 1);
    }

    if (args[0].compare("reset") == 0)
    {
        h89.reset();
        return "ok";
    }

    if (args[0].compare("mount") == 0)
    {
        if (args.size() < 3)
        {
            return "error syntax: " + cmd;
        }

        GenericDiskDrive* drv = findDrive(args[1]);

        if (drv == nullptr)
        {
            return "error nodrive: " + args[1];
        }

        drv->insertDisk(SectorFloppyImage::getDiskette(drv, PropertyUtil::shiftArgs(args, 2)));
        return "ok";
    }

    if (args[0].compare("getdisks") == 0)
    {
        int                          count = 0;
        std::vector<DiskController*> devs  = h89.getIO().getDiskDevices();
        std::ostringstream           resp;
        resp << "ok ";

        for (int x = 0; x < devs.size(); ++x)
        {
            DiskController* dev = devs[x];

            if (dev != nullptr)
            {
                std::vector<GenericDiskDrive*> drives = dev->getDiskDrives();

                for (int y = 0; y < drives.size(); ++y)
                {
                    GenericDiskDrive* drv = drives[y];

                    if (drv != nullptr)
                    {
                        if (count++ > 0)
                        {
                            resp << ';';
                        }

                        std::string media = drv->getMediaName();
                        // media = media.replaceAll(" ", "%20");
                        resp << dev->getDriveName(y) << "=" << media;
                    }
                }
            }
        }

        return resp.str();
    }

    if (args[0].compare("dump") == 0 && args.size() > 1)
    {
        if (args[1].compare("cpu") == 0)
        {
            std::string dump = "ok " + h89.getCPU().dumpDebug();
            return cleanse(dump);
        }

        if (args[1].compare("mach") == 0)
        {
            std::string dump = "ok " + h89.dumpDebug();
            return cleanse(dump);
        }

        if (args[1].compare("disk") == 0 && args.size() > 2)
        {
            DiskController* dev = findDiskCtrlr(args[2]);

            if (dev == nullptr)
            {
                return "error no device " + args[2];
            }

            std::string dump = "ok " + dev->dumpDebug();
            return cleanse(dump);
        }
    }

    return "error badcmd: " + cmd;
}

std::string
H89Operator::handleCommand(std::string cmd)
{
    h89.systemMutexAcquire();
    std::string resp = executeCommand(cmd);
    h89.systemMutexRelease();
    return resp;
}
