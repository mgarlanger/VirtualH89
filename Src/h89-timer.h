/// \file h89-timer.h
///
/// \date   Mar 26, 2009
/// \author Mark Garlanger
///

#ifndef H89TIMER_H_
#define H89TIMER_H_

#include "EventHandler.h"

class CPU;
class InterruptController;

///
/// \class H89Timer
///
/// \brief %H89 2 mSec timer
///
///
class H89Timer: public EventHandler
{
  public:
    H89Timer(CPU *cpu, InterruptController *ic, unsigned char intlvl = 1);
    H89Timer(unsigned char intlvl = 1);
    virtual ~H89Timer();
    virtual void setCPU(CPU *cpu);
    virtual void setInterruptController(InterruptController *cpu);
    virtual int handleSignal(int signum);

    virtual void enableINT();
    virtual void disableINT();

  private:
    CPU *cpu_m;
    InterruptController *ic_m;
    bool intEnabled_m;

    int count_m;
    unsigned char intLevel;

};

#endif // H89TIMER_H_
