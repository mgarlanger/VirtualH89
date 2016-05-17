///
/// \name InterruptController.h
///
///
/// \date Feb 7, 2016
/// \author Mark Garlanger
///

#ifndef INTERRUPTCONTROLLER_H_
#define INTERRUPTCONTROLLER_H_

#include "h89Types.h"

/// \class InterruptController
///
/// \brief Interrupt Controller Logic
///
/// Although the H89 did not have an interrupt chip, it had special logic to generate
/// an RST xx instruction for each interrupt. Heath's soft-sectored controller and
/// MMS's 77316 both modified the logic to either generate an RST for interrupts
/// and an EI instruction when DRQ has been raised.
///

class CPU;

class InterruptController
{
  protected:
    BYTE intLevel_m;
    CPU* cpu_m;

    virtual void setINTLine();

  public:
    InterruptController(CPU* cpu);
    virtual ~InterruptController();

    virtual void raiseInterrupt(BYTE level);
    virtual void lowerInterrupt(BYTE level);

    virtual void setDrq(bool raise);
    virtual void setIntrq(bool raise);
    virtual void blockInterrupts(bool block);

    // reading instructions for interrupts
    virtual BYTE readDataBus();
};



#endif // INTERRUPTCONTROLLER_H_
