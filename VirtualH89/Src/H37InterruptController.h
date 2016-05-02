///
///  \file H37InterruptController.h
///
///  \brief Interrupt controller for the H37 soft-sectored disk controller
///
///  \author Mark Garlanger
///  \date  Mar. 17, 2016
///

#ifndef H37_INTERRUPT_CONTROLLER_H
#define H37_INTERRUPT_CONTROLLER_H

#include "InterruptController.h"


class H37InterruptController: public InterruptController
{

  protected:
    bool intrqRaised_m;
    bool drqRaised_m;
    bool interruptsBlocked_m;

    virtual void setINTLine();

  public:
    H37InterruptController(CPU* cpu);
    virtual ~H37InterruptController();

    // reading instructions for interrupts
    virtual BYTE readDataBus();

    virtual void setDrq(bool raise);
    virtual void setIntrq(bool raise);
    virtual void blockInterrupts(bool block);

};



#endif // H37_INTERRUPT_CONTROLLER_H
