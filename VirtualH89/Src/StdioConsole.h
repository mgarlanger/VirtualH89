/// \file StdioConsole.h
///
/// \date Feb 6, 2016
/// \author Douglas Miller
///

#ifndef STDIOCONSOLE_H_
#define STDIOCONSOLE_H_


#include "Console.h"

/// \cond
#include <assert.h>
/// \endcond

/// \brief StdioConsole
///
///
class StdioConsole: public Console
{
  public:
    StdioConsole(int    argc,
                 char** argv);
    virtual ~StdioConsole() override;

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
};

#endif // STDIOCONSOLE_H_
