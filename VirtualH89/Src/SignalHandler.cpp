/// \file SignalHandler.cpp
///
/// \date Mar 26, 2009
/// \author Mark Garlanger
///

#include "SignalHandler.h"

#include "EventHandler.h"

SignalHandler* SignalHandler::_inst;
EventHandler*  SignalHandler::signalHandlers[NSIG];

SignalHandler::SignalHandler()
{

}

SignalHandler::~SignalHandler()
{

}

SignalHandler*
SignalHandler::instance(void)
{
    if (!_inst)
    {
        _inst = new SignalHandler();
    }

    return (_inst);
}

EventHandler*
SignalHandler::registerHandler(int           signum,
                               EventHandler* evtHandler)
{
    static struct sigaction newact;

    EventHandler*           orig_evtHandler = signalHandlers[signum];

    signalHandlers[signum] = evtHandler;
    newact.sa_handler      = dispatcher;

    sigaction(signum, &newact, 0);

    return (orig_evtHandler);
}

int
SignalHandler::removeHandler(int signum)
{
    static struct sigaction newact;

    newact.sa_handler      = SIG_IGN;
    sigaction(signum, &newact, 0);

    signalHandlers[signum] = 0;

    return (signum);
}


void
SignalHandler::dispatcher(int signum)
{
    if (signalHandlers[signum] != 0)
    {
        signalHandlers[signum]->handleSignal(signum);
    }
}
