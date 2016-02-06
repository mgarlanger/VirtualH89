/// \file Console.h
///
/// \date Feb 6, 2016
/// \author Douglas Miller
///

#ifndef CONSOLE_H_
#define CONSOLE_H_

#include "Terminal.h"
#include <string>

/// \class Console
///
/// \brief  interface Console.
///
/// Base class for generic terminal.
///
class Console : public Terminal
{
  public:
    Console(int argc, char **argv);
    virtual ~Console();

    virtual void run() = 0;
    virtual void init() = 0;
    virtual void reset() = 0;
    std::string command(std::string cmd);

    // obsolete? deprecated?
    virtual void processCharacter(char ch) = 0;
    virtual void display() = 0;
    virtual void keypress(char ch) = 0;
    virtual bool checkUpdated() = 0;

  private:

};

#endif // CONSOLE_H_
