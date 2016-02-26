/// \file StdioProxyConsole.cpp
///
/// A console replacement that supports an external process as the H19.
/// Also supports a out-of-band channel for command/control messages.
/// Uses the process stdin and stout for communication with the virtual H19.
/// H19 traffic has the high bit set, while oob traffic has the high bit clear.
/// Thus, only 7-bit ASSCII is supported for each channel.
///
/// \date Feb 6, 2016
/// \author Douglas Miller
///

#include "StdioProxyConsole.h"

#include <stdio.h>
#include <unistd.h>
#include <pthread.h>
#include <signal.h>
#include <string.h>
#include <termios.h>
#include <unistd.h>
#include <sstream>

#include "H89.h"
#include "h89-io.h"
#include "logger.h"
#include "H89Operator.h"

/// \brief StdioProxyConsole
///
///
StdioProxyConsole::StdioProxyConsole(int argc, char** argv): Console(argc, argv)
{
    int          c;
    extern char* optarg;

    // TODO: interpret args to get pipe/fifo for commands.
    while ((c = getopt(argc, argv, "d:")) != EOF)
    {
        switch (c)
        {
            case 'd':
                sleep((unsigned int) strtol(optarg, NULL, 0));
                break;
        }
    }

    op_m = new H89Operator();
}

StdioProxyConsole::~StdioProxyConsole() {
}

void
StdioProxyConsole::init() {
}

void
StdioProxyConsole::reset() {
}

void
StdioProxyConsole::display() {
}

void
StdioProxyConsole::processCharacter(char ch) {
}

void
StdioProxyConsole::keypress(char ch)
{
    // try not to overrun the UART, but do not wait too long.
    int timeout = 2000;
    int sleep   = 100;

    while (!sendReady() && timeout > 0)
    {
        usleep(sleep);
        timeout -= sleep;
    }

    sendData(ch);
}

void
StdioProxyConsole::receiveData(BYTE ch)
{
    fputc(ch | 0x80, stdout);
    fflush(stdout);
}

bool
StdioProxyConsole::checkUpdated()
{
    return false;
}

unsigned int
StdioProxyConsole::getBaudRate()
{
    return SerialPortDevice::DISABLE_BAUD_CHECK;
}

void
StdioProxyConsole::run()
{
    static char buf[1024];
    int         x = 0;
    int         c;

    // setbuf(stdin, NULL);

    while ((c = fgetc(stdin)) != EOF)
    {
        if ((c & 0x80) != 0)
        {
            c &= 0x7f;
            keypress(c);
        }
        else if (c == 0x0a)
        {
            if (x >= sizeof(buf))
            {
                x = sizeof(buf) - 1;
            }

            buf[x] = '\0';
            x      = 0;
            std::string resp = op_m->handleCommand(buf);
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
