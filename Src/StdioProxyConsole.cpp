/// \file StdioProxyConsole.cpp
///
/// \date Feb 6, 2016
/// \author Douglas Miller
///

#include "H89.h"
#include "h89-io.h"
#include "StdioProxyConsole.h"
#include "logger.h"
#include <stdio.h>
#include <unistd.h>
#include <pthread.h>
#include <signal.h>
#include <string.h>
#include <termios.h>
#include <unistd.h>
#include <sstream>

#include "DiskController.h"
#include "GenericDiskDrive.h"

/// \brief StdioProxyConsole
///
///
StdioProxyConsole::StdioProxyConsole(int argc, char **argv): Console(argc, argv)
{
    int c;
    extern char *optarg;

    // TODO: interpret args to get pipe/fifo for commands.
    while ((c = getopt(argc, argv, "d:")) != EOF)
    {
        switch (c)
        {
        case 'd':
            sleep(strtol(optarg, NULL, 0));
            break;
        }
    }
}

StdioProxyConsole::~StdioProxyConsole() {}

void StdioProxyConsole::init() {}

void StdioProxyConsole::reset() {}

void StdioProxyConsole::display() {}

void StdioProxyConsole::processCharacter(char ch) {}

void StdioProxyConsole::keypress(char ch)
{
    sendData(ch);
}

void StdioProxyConsole::receiveData(BYTE ch)
{
    fputc(ch | 0x80, stdout);
    fflush(stdout);
}

bool StdioProxyConsole::checkUpdated()
{
    return false;
}

unsigned int StdioProxyConsole::getBaudRate()
{
    return SerialPortDevice::DISABLE_BAUD_CHECK;
}

std::string StdioProxyConsole::handleCommand(std::string cmd)
{
    // TODO: implement commands...
    if (cmd.compare("getdisks") == 0)
    {
        int count = 0;
        std::vector<DiskController *> devs = h89.getIO().getDiskDevices();
        std::ostringstream resp;

        for (int x = 0; x < devs.size(); ++x)
        {
            DiskController *dev = devs[x];

            if (dev != NULL)
            {
                std::vector<GenericDiskDrive *> drives = dev->getDiskDrives();

                for (int y = 0; y < drives.size(); ++y)
                {
                    GenericDiskDrive *drv = drives[y];

                    if (drv != NULL)
                    {
                        if (count++ > 0)
                        {
                            resp << ';';
                        }

                        resp << dev->getDeviceName() << '-' <<
                             (y + 1) << "=\"" <<
                             drv->getMediaName() << '"';
                    }
                }
            }
        }

        return resp.str();
    }

    else
    {
        return "badcommand: " + cmd;
    }
}

void StdioProxyConsole::run()
{
    static char buf[1024];
    int x = 0;
    int c;

    //setbuf(stdin, NULL);

    while ((c = fgetc(stdin)) != EOF)
    {
        if ((c & 0x80) != 0)
        {
            keypress(c);
        }

        else if (c == 0x0a)
        {
            if (x >= sizeof(buf))
            {
                x = sizeof(buf) - 1;
            }

            buf[x] = '\0';
            x = 0;
            std::string resp = handleCommand(buf);
            fputs(resp.c_str(), stdout);
            fputc('\n', stdout);
            fflush(stdout);
        }

        else
        {
            if (x < sizeof(buf))
            {
                buf[x++] = c;
            }
        }
    }
}
