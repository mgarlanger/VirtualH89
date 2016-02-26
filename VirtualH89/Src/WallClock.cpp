///
/// \file WallClock.cpp
///
/// \date Apr 29, 2009
///
/// \author Mark Garlanger
///

#include "WallClock.h"

#include "config.h"
#include "ClockUser.h"
#include "logger.h"
#include <cstddef>


WallClock* WallClock::_inst = nullptr;


WallClock::WallClock()
{

}

WallClock::~WallClock()
{

}

WallClock*
WallClock::instance(void)
{
    if (!_inst)
    {
        _inst = new WallClock();
    }

    return (_inst);
}

void
WallClock::addTimerEvent()
{
#if TWOMSEC

    /// \todo Properly handle other CPU Speeds.
    if (ticks_m > 4096)
    {
        clock_m += ticks_m;
    }
    else
    {
        /// \todo determine if we need to notify all users here too for (4096 - ticks_m)
        ///       number of ticks.

        clock_m += 4096;
    }

#else
    clock_m += 4096 * 5;
#endif
    ticks_m  = 0;
}

void
WallClock::addTicks(unsigned ticks)
{
    ticks_m += ticks;

    for (std::list<ClockUser*>::iterator it = users_m.begin(); it != users_m.end(); ++it)
    {
        (*it)->notification(ticks);;
    }
}

long long unsigned int
WallClock::getClock()
{
    return (clock_m + ticks_m);
}

long long unsigned int
WallClock::getElapsedTime(long long unsigned int origTime)
{
    return (clock_m + ticks_m - origTime);
}

void
WallClock::printTime(FILE* file)
{
    // determine seconds, millisec, microseconds..
    /// \todo Properly handle other CPU Speeds.

    unsigned long long time     = clock_m + ticks_m;
    unsigned long long millisec = time >> 11;
    unsigned long long seconds  = millisec / 1000;

    millisec %= 1000;

    fprintf(file, "%05lld.%03lld:%04lld - ", seconds, millisec, time & 0x7ff);
}

bool
WallClock::registerUser(ClockUser* user)
{
    users_m.push_back(user);
    return true;
}

bool
WallClock::unregisterUser(ClockUser* user)
{
    users_m.remove(user);
    return true;
}

void
WallClock::updateTicksPerSecond(unsigned long ticks)
{
    ticksPerSecond = ticks;
}
