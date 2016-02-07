/// \file StdioProxyConsole.cpp
///
/// \date Feb 6, 2016
/// \author Douglas Miller
///

#include "StdioProxyConsole.h"
#include "logger.h"
#include <stdio.h>
#include <unistd.h>
#include <pthread.h>
#include <signal.h>
#include <string.h>
#include <termios.h>
#include <unistd.h>

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
    fputc(ch, stdout);
    fflush(stdout);
}

bool StdioProxyConsole::checkUpdated()
{
    return false;
}

unsigned int StdioProxyConsole::getBaudRate()
{
    return 0;
}

void StdioProxyConsole::run()
{
    int c;

    //setbuf(stdin, NULL);

    while ((c = fgetc(stdin)) != EOF)
    {
        keypress(c);
    }
}
