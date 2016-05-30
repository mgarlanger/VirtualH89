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


#include "h89Types.h"

/// \cond
#include <vector>
#include <memory>
/// \endcond

class GppListener: public std::enable_shared_from_this<GppListener>
{
  public:
    GppListener(BYTE bits = 0xff);

    static void addListener(GppListener* lstr);
    static void notifyListeners(BYTE gpo, BYTE diffs);

  protected:
    virtual void gppNewValue(BYTE gpo) = 0;
    BYTE                             interestedBits_m;

  private:
    static std::vector<GppListener*> notifications;
};

#endif // GPPLISTENER_H_
