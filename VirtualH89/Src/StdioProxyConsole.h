/// \file StdioProxyConsole.h
///
/// A console replacement that supports an external process as the H19.
/// Also supports a out-of-band channel for command/control messages.
/// Typically, this is selected by commandline options for main(),
/// with the parent process as the H19 emulation.
///
/// \date Feb 6, 2016
/// \author Douglas Miller
///

#ifndef STDIOPROXYCONSOLE_H_
#define STDIOPROXYCONSOLE_H_


#include "Console.h"

/// \cond
#include <assert.h>
/// \endcond

class H89Operator;

/// \brief StdioProxyConsole
///
///
class StdioProxyConsole: public Console
{
  public:
    StdioProxyConsole(int    argc,
                      char** argv);
    virtual ~StdioProxyConsole() override;

    virtual void init() override;
    virtual void reset() override;
    virtual void display() override;
    virtual void processCharacter(char ch) override;
    virtual void keypress(char ch) override;
    virtual void receiveData(BYTE) override;
    virtual bool checkUpdated() override;
    virtual unsigned int getBaudRate() override;
    virtual void run() override;

  private:
    H89Operator* op_m;
    bool         logConsole;
};

#endif // STDIOPROXYCONSOLE_H_
