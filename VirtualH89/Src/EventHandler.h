/// \file EventHandler.h
///
/// \date Mar 26, 2009
/// \author Mark Garlanger
///

#ifndef EVENTHANDLER_H_
#define EVENTHANDLER_H_

class EventHandler
{
  public:
    virtual int handleSignal(int signum) = 0;

    EventHandler();

    virtual ~EventHandler();
};

#endif // EVENTHANDLER_H_
