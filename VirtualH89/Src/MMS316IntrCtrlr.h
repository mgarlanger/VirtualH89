///
/// \name MMS316IntrCtrlr.h
///
/// Implementation of the H89 interrupt controller circuits, when augmented by the
/// MMS 77316 floppy disk controller. Used only if the MMS77316 is installed.
///
/// \date Feb 8, 2016
/// \author Douglas Miller
///

#ifndef MMS316INTRCTRLR_H_
#define MMS316INTRCTRLR_H_

#include "InterruptController.h"

class MMS77316;

/// \class MMS316IntrCtrlr
///
/// \brief Daisy-chaining Interrupt Controller Logic
///
///

class MMS316IntrCtrlr: public InterruptController
{
  private:
    MMS77316* m316_m;

  public:
    MMS316IntrCtrlr(InterruptController* ic,
                    MMS77316*            m316);
    virtual ~MMS316IntrCtrlr();

    virtual void raiseInterrupt(BYTE level);
    virtual void lowerInterrupt(BYTE level);

    // reading instructions for interrupts
    virtual BYTE readDataBus();
};



#endif // MMS316INTRCTRLR_H_
