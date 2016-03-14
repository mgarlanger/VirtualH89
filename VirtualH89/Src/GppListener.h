///
/// \file GppListener.h
///
/// Object to facilitate interested parties of GPIO bit changes to get
/// notified when bits change.
///
/// \date Mar 05, 2016
/// \author Douglas Miller
///
#ifndef GPPLISTENER_H_
#define GPPLISTENER_H_

#include <vector>

#include "h89Types.h"

class GppListener
{
  public:
    GppListener(): interestedBits(0xff) {
    }
    GppListener(BYTE bits): interestedBits(bits) {
    }
    static void addListener(GppListener* lstr);
    static void notifyListeners(BYTE gpo, BYTE diffs);
  protected:
    virtual void gppNewValue(BYTE gpo) = 0;
    BYTE                             interestedBits;
  private:
    static std::vector<GppListener*> notifications;
};

#endif // GPPLISTENER_H_
