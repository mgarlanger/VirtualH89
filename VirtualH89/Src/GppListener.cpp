///
/// \file GppListener.cpp
///
/// Register parties interested in GPIO bits, and notify them whenever bits change.
///
/// \date Mar 05, 2016
/// \author Douglas Miller
///
#include "GppListener.h"

#include <stdio.h>

std::vector<GppListener*> GppListener::notifications;

void
GppListener::addListener(GppListener* lstr) {
    // If we have more than one, need vector<>, array, or ...
    notifications.push_back(lstr);
}

void
GppListener::notifyListeners(BYTE gpo, BYTE diffs) {
    unsigned int x;
    for (x = 0; x < notifications.size(); ++x)
    {
        if ((notifications[x]->interestedBits & diffs) != 0)
        {
            notifications[x]->gppNewValue(gpo);
        }
    }
}
