/// \file WallClock.h
///
/// \date Apr 29, 2009
/// \author Mark Garlanger
///

#ifndef WALLCLOCK_H_
#define WALLCLOCK_H_

#include <cstdio>
#include <list>
//#include <atomic>

class ClockUser;

/// \class WallClock
///
/// \brief Clock counting CPU cycles.
///
/// Provides a 'real-time' clock allowing times down to a cpu cycle.
///
/// WallClock is to provide the overall 'real-time' down to a cpu cycle.
///
/// This is a singleton to make sure everyone is using the one and only instance.
///
/// Note: The name is a bit misleading. This does not have anything to do with actual time,
///       but the virtual time as seen by the CPU.
class WallClock
{
  private:
    long long unsigned int clock_m = 0;
//    std::atomic_ullong clock_m = 0;
    unsigned int ticks_m = 0;
//    std::atomic_uint ticks_m = 0;
    static WallClock *_inst;

    unsigned long ticksPerSecond = 2048000;

    WallClock();
    virtual ~WallClock();

    /// use C++11 to avoid having to define copy constructor
    WallClock(WallClock const&) = delete;
    WallClock& operator=(WallClock const&) = delete;

    std::list<ClockUser *> users_m;

  public:
    bool registerUser(ClockUser *user);
    bool unregisterUser(ClockUser *user);

    static WallClock *instance(void);

    void addTimerEvent();

    void updateTicksPerSecond(unsigned long ticks);
    unsigned long getTicksPerSecond()
    {
        return ticksPerSecond;
    }

    void addTicks(unsigned ticks);

    long long unsigned int getClock();

    long long unsigned int getElapsedTime(long long unsigned int origTime);

    void printTime(FILE *file);

    bool addCallback(ClockUser *user, unsigned long long timeUs);
};

#endif // WALLCLOCK_H_
