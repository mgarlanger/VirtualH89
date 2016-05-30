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

#include "H89.h"
#include "h89-io.h"
#include "logger.h"
#include "H89Operator.h"

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

extern const char* getopts;

/// \brief StdioProxyConsole
///
///
StdioProxyConsole::StdioProxyConsole(int    argc,
                                     char** argv):
    Console(argc, argv),
    logConsole(false)
{
    int          c;
    extern char* optarg;
    extern int   optind;

    // TODO: properly share getopt string so both main and this
    // (and other modules) can ignore eachother's options...

    optind = 1;
    while ((c = getopt(argc, argv, getopts)) != EOF)
    {
        switch (c)
        {
            case 'l':
                logConsole = true;
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
    int timeout = 4000;
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
    if (logConsole)
    {
        fputc(ch, console_out);
        if (ch == '\n')
        {
            fflush(console_out);
        }
    }
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
