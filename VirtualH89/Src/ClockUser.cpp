///
/// \name clockUser.cpp
///
/// \brief notification class for clock ticks
///
/// \date Aug 11, 2012
/// \author Mark Garlanger
///

#include "ClockUser.h"

#include "WallClock.h"

ClockUser::ClockUser()
{
    WallClock::instance()->registerUser(this);
}

ClockUser::~ClockUser()
{
    WallClock::instance()->unregisterUser(this);
}
