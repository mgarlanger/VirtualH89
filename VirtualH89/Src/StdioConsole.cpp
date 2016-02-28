/// \file StdioConsole.cpp
///
/// \date Feb 6, 2016
/// \author Douglas Miller
///

#include "StdioConsole.h"

#include "logger.h"
#include <stdio.h>
#include <unistd.h>
#include <pthread.h>
#include <signal.h>
#include <string.h>
#include <termios.h>
#include <unistd.h>

/// \brief StdioConsole
///
///
StdioConsole::StdioConsole(int    argc,
                           char** argv): Console(argc, argv)
{
}

StdioConsole::~StdioConsole() {
}

void
StdioConsole::init() {
}

void
StdioConsole::reset() {
}

void
StdioConsole::display() {
}

void
StdioConsole::processCharacter(char ch) {
}

void
StdioConsole::keypress(char ch)
{
    if (ch == 0x0a)
    {
        ch = 0x0d;
    }

    sendData(ch);
}

void
StdioConsole::receiveData(BYTE ch)
{
    static int mode = 0;

    if (mode > 0)
    {
        if (ch == 'Z')
        {
            sendData('K');
            mode = -1;
        }
        else
        {
            mode = 0;
        }
    }
    else if (mode == 0 && ch == 0x1b)
    {
        ++mode;
    }
    else
    {
        fputc(ch, stdout);
        fflush(stdout);
    }
}

bool
StdioConsole::checkUpdated()
{
    return false;
}
unsigned int
StdioConsole::getBaudRate()
{
    return SerialPortDevice::DISABLE_BAUD_CHECK;
}
void
StdioConsole::run()
{
    int            ret;
    int            c;
    struct termios termios0;
    struct termios termios;

    fprintf(stdout, "Press ESC to quit.\n");
    fflush(stdout);

    setbuf(stdin, NULL);
    ret = tcgetattr(0, &termios);

    if (ret == 0)
    {
        memcpy(&termios0, &termios, sizeof(termios0));
        termios.c_lflag &= ~ICANON;
        termios.c_lflag &= ~ECHO;
        ret              = tcsetattr(0, 0, &termios);
    }

    while ((c = fgetc(stdin)) != EOF)
    {
        if (c == 0x1b)
        {
            break;
        }

        keypress(c);
    }

    if (ret == 0)
    {
        tcsetattr(0, 0, &termios0);
    }
}
