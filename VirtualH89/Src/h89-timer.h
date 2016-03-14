/// \file h89-timer.h
///
/// \date   Mar 26, 2009
/// \author Mark Garlanger
///

#ifndef H89TIMER_H_
#define H89TIMER_H_

#include <pthread.h>

#include "EventHandler.h"
#include "GppListener.h"

class CPU;
class InterruptController;

///
/// \class H89Timer
///
/// \brief %H89 2 mSec timer
///
///
class H89Timer: public EventHandler, public GppListener
{
  public:
    H89Timer(CPU*          cpu,
             unsigned char intlvl = 1);
    H89Timer(unsigned char intlvl = 1);
    virtual ~H89Timer();
    virtual void setCPU(CPU* cpu);
    virtual int handleSignal(int signum);

    void reset();
    void start();

  private:
    virtual void gppNewValue(BYTE gpo);
    static const BYTE h89timer_gpp2msIntEnBit_c = 0b00000010;
    CPU*              cpu_m;
    bool              intEnabled_m;

    int               count_m;
    unsigned char     intLevel;
    pthread_t         thread;

};

#endif // H89TIMER_H_
