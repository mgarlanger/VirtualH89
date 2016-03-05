/// \file cpu.h
///
/// \date Mar 7, 2009
/// \author Mark Garlanger
///

#ifndef CPU_H_
#define CPU_H_


#include <string>

#include "config.h"
#include "h89Types.h"

class AddressBus;

///
/// \brief  Abstract processor.
///
/// Basic processor abstraction class.
///
class CPU
{
  public:
    static const unsigned long NO_INTR_INST = ((unsigned long) -1);
    typedef unsigned long intrCheck (void* arg,
                                     int   level);
  private:

  public:

    CPU();
    virtual ~CPU();

    virtual void addClockTicks(void)           = 0;
    virtual BYTE execute(WORD numInst = 0)     = 0;
    virtual void reset(void)                   = 0;
    virtual BYTE step(void)                    = 0;
    virtual void raiseINT(void)                = 0;
    virtual void lowerINT(void)                = 0;
    virtual void raiseNMI(void)                = 0;
    virtual void setAddressBus(AddressBus* ab) = 0;
    virtual void setSpeed(bool fast)           = 0;
    virtual void continueRunning(void)         = 0;
    virtual void waitState(void)               = 0;
    virtual std::string dumpDebug()            = 0;

};

#endif // CPU_H_
