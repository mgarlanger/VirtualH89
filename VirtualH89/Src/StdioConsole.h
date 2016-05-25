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
    virtual ~StdioConsole();

    virtual void init();
    virtual void reset();
    virtual void display();
    virtual void processCharacter(char ch);
    virtual void keypress(char ch);
    virtual void receiveData(BYTE);
    virtual bool checkUpdated();
    virtual unsigned int getBaudRate();
    virtual void run();

  private:
};

#endif // STDIOCONSOLE_H_
