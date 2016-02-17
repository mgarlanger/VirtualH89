///
/// \name InterruptController.h
///
///
/// \date Feb 7, 2016
/// \author Mark Garlanger
///

#ifndef INTERRUPTCONTROLLER_H_
#define INTERRUPTCONTROLLER_H_

#include "config.h"
#include "h89Types.h"

/// \class InterruptController
///
/// \brief Interrupt Controller Logic
///
/// Although the H89 did not have an interrupt chip, it had special logic to generate
/// an RST xx instruction for each interrupt. At least one third-party add-on card (mms77316)
/// modified the logic to either generate the RST xx or an EI instruction.
///

class CPU;

class InterruptController
{
  private:
    BYTE  intLevel_m;
    CPU*   cpu_m;

  public:
    InterruptController(CPU *cpu);
    virtual ~InterruptController();

    virtual void raiseInterrupt(BYTE level);
    virtual void lowerInterrupt(BYTE level);

    // reading instructions for interrupts
    virtual BYTE readDataBus();
};



#endif // INTERRUPTCONTROLLER_H_
