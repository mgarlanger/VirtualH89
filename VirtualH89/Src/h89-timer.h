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
class Computer;
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
    H89Timer(Computer*     computer,
             CPU*          cpu,
             unsigned char intlvl = 1);
    virtual ~H89Timer();

    virtual int handleSignal(int signum);

    void reset();
    void start();

  private:
    virtual void gppNewValue(BYTE gpo);
    static const BYTE h89timer_gpp2msIntEnBit_c = 0b00000010;
    Computer*         computer_m;
    CPU*              cpu_m;
    bool              intEnabled_m;

    unsigned long     count_m;
    unsigned char     intLevel;
    pthread_t         thread;

};

#endif // H89TIMER_H_
