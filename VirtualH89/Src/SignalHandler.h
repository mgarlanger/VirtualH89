/// \file SignalHandler.h
///
/// \date Mar 26, 2009
/// \author Mark Garlanger
///

#ifndef SIGNALHANDLER_H_
#define SIGNALHANDLER_H_

class EventHandler;

/// \cond
#include <csignal>
/// \endcond

/// Class to handle signals. This is a singleton.
///
class SignalHandler
{
  public:
    static SignalHandler* instance(void);

    EventHandler* registerHandler(int           signum,
                                  EventHandler* evtHandler);
    int removeHandler(int signum);

  private:
    SignalHandler(void);
    ~SignalHandler(void);

    static SignalHandler* _inst;

    static void dispatcher(int signum);

    static EventHandler*  signalHandlers[NSIG];
};

#endif // SIGNALHANDLER_H_
