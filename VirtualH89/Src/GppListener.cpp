///
/// \file GppListener.cpp
///
/// Register parties interested in GPIO bits, and notify them whenever bits change.
///
/// \date Mar 05, 2016
/// \author Douglas Miller
///
#include "GppListener.h"


using namespace std;

std::vector<GppListener*> GppListener::notifications;


GppListener::GppListener(BYTE bits): interestedBits_m(bits)
{

}

void
GppListener::addListener(GppListener* listener)
{
    // If we have more than one, need vector<>, array, or ...
    notifications.push_back(listener);
}

void
GppListener::notifyListeners(BYTE gpo,
                             BYTE diffs)
{

    for (GppListener* listener : notifications)
    {
        if ((listener->interestedBits_m & diffs) != 0)
        {
            listener->gppNewValue(gpo);
        }
    }

}
