/// \file cpu.h
///
/// \date Mar 7, 2009
/// \author Mark Garlanger
///

#ifndef CPU_H_
#define CPU_H_

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
  private:

  public:
    CPU();
    virtual ~CPU();

    virtual void addClockTicks(void) = 0;
    virtual BYTE execute(WORD numInst = 0) = 0;
    virtual void reset(void) = 0;
    virtual BYTE step(void) = 0;
    virtual void raiseINT(int level) = 0;
    virtual void lowerINT(int level) = 0;
    virtual void raiseNMI(void) = 0;
    virtual void setAddressBus(AddressBus *ab) = 0;
    virtual void setSpeed(bool fast) = 0;
    virtual void continueRunning(void) = 0;

};

#endif // CPU_H_
