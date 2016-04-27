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

class CPU;

/// \class MMS316IntrCtrlr
///
/// \brief Daisy-chaining Interrupt Controller Logic
///
///

class MMS316IntrCtrlr: public InterruptController
{
  protected:
    bool drqRaised_m;
    bool intrqRaised_m;

  public:
    MMS316IntrCtrlr(CPU* cpu);
    virtual ~MMS316IntrCtrlr();

    void setINTLine();

    // reading instructions for interrupts
    virtual BYTE readDataBus();
    virtual void setDrq(bool raise);
    virtual void setIntrq(bool raise);

};



#endif // MMS316INTRCTRLR_H_
