/// \file h89-timer.cpp
///
/// \date Mar 26, 2009
/// \author Mark Garlanger
///

#include "h89-timer.h"


#include "computer.h"
#include "cpu.h"
#include "SignalHandler.h"
#include "WallClock.h"
#include "logger.h"
#include "config.h"

/// \cond
#include <sys/time.h>
#include <signal.h>
/// \endcond


static const int TimerInterval_c = 2000;


H89Timer::H89Timer(Computer*     computer,
                   CPU*          cpu,
                   unsigned char intlvl): GppListener(h89timer_gpp2msIntEnBit_c),
                                          computer_m(computer),
                                          cpu_m(cpu),
                                          intEnabled_m(false),
                                          count_m(0),
                                          intLevel(intlvl),
                                          thread(0)
{
    // We need to start up the timer since it performs two tasks, it always provide the cpu
    // with extra clock ticks to accurately emulate the speed of the processor.
    // Plus, if interrupts are enabled, it will interrupt the cpu with the timer tick.

    debugss(ssTimer, INFO, "\n");

    SignalHandler::instance()->registerHandler(SIGALRM, this);
    GppListener::addListener(this);
}


H89Timer::~H89Timer()
{
    static struct itimerval tim;

    debugss(ssTimer, INFO, "\n");

    SignalHandler::instance()->removeHandler(SIGALRM);

    tim.it_value.tv_sec  = 0;
    tim.it_value.tv_usec = 0;
    setitimer(ITIMER_REAL, &tim, nullptr);
}

void
H89Timer::reset()
{
    intEnabled_m = false;
    count_m      = 0;
}

void
H89Timer::start()
{
    static struct itimerval tim;

    thread                   = pthread_self();

    tim.it_value.tv_sec      = 0;
    tim.it_value.tv_usec     = TimerInterval_c;

    tim.it_interval.tv_sec   = 0;
    tim.it_interval.tv_usec  = TimerInterval_c;

#if TEN_X_SLOWER
    tim.it_value.tv_usec    *= 20;
    tim.it_interval.tv_usec *= 20;
#endif

    setitimer(ITIMER_REAL, &tim, nullptr);

}

int
H89Timer::handleSignal(int signum)
{
    debugss(ssTimer, ALL, "Timer Handle Signal\n");

    if (signum != SIGALRM)
    {
        debugss(ssTimer, ERROR, "signum != SIGALRM: %d\n", signum);

        return 0;
    }
    if (thread == 0)
    {
        // real thread is not set, can't do much else for now.
        return 0;
    }
    if (thread != pthread_self())
    {
        // pass the signal to the correct thread.
        pthread_kill(thread, SIGALRM);
        return 0;
    }

    count_m++;

    WallClock::instance()->addTimerEvent();

    if (cpu_m)
    {
        debugss(ssTimer, VERBOSE, "adding clock ticks\n");

        // must always give the CPU more cycles.
        cpu_m->addClockTicks();

        // Only if interrrupt is enabled.
        if (intEnabled_m)
        {
            debugss(ssTimer, VERBOSE, "raising Interrupt\n");

            computer_m->raiseINT(intLevel);
        }
    }
    else
    {
        debugss(ssTimer, ERROR, "cpu_m is NULL\n");
    }

    return 0;
}

void
H89Timer::gppNewValue(BYTE gpo)
{
    intEnabled_m = ((gpo & h89timer_gpp2msIntEnBit_c) != 0);
}
