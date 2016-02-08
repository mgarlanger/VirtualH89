///
/// \name MMS316IntrCtrlr.h
///
///
/// \date Feb 8, 2016
/// \author Douglas Miller
///

#ifndef MMS316INTRCTRLR_H_
#define MMS316INTRCTRLR_H_

#include "config.h"
#include "h89Types.h"
#include "InterruptController.h"
#include "mms77316.h"

/// \class MMS316IntrCtrlr
///
/// \brief Daisy-chaining Interrupt Controller Logic
///
///

class MMS316IntrCtrlr: public InterruptController
{
  private:
    InterruptController *ic_m;
    MMS77316 *m316_m;

  public:
    MMS316IntrCtrlr(InterruptController *ic, MMS77316 *m316);
    virtual ~MMS316IntrCtrlr();

    virtual void raiseInterrupt(BYTE level);
    virtual void lowerInterrupt(BYTE level);

    // reading instructions for interrupts
    virtual BYTE readDataBus();
};



#endif // MMS316INTRCTRLR_H_
