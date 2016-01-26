/// \file Terminal.h
///
/// \date Apr 12, 2009
/// \author Mark Garlanger
///

#ifndef TERMINAL_H_
#define TERMINAL_H_

#include "SerialPortDevice.h"

/// \class Terminal
///
/// \brief  Base %Terminal.
///
/// Base class for generic terminal.
///
class Terminal : public SerialPortDevice
{
  public:
    Terminal();
    virtual ~Terminal();

    virtual void init() = 0;
    virtual void reset() = 0;

    virtual void processCharacter(char ch) = 0;
    virtual void display() = 0;
    virtual void keypress(char ch) = 0;
    virtual bool checkUpdated() = 0;

  private:

};

#endif // TERMINAL_H_
