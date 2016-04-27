///
/// \name computer.h
/// \brief Base computer class.
///
/// \date Jul 16, 2012
/// \author Mark Garlanger
///

#ifndef COMPUTER_H_
#define COMPUTER_H_

#include "h89Types.h"

class AddressBus;

/// \class Computer
///
class Computer
{
  private:

  public:
    Computer();
    virtual ~Computer();

    virtual void reset()                = 0;
    virtual BYTE run()                  = 0;

    virtual void init()                 = 0;

    virtual void keypress(BYTE ch)      = 0;
    virtual void display()              = 0;

    virtual void raiseINT(int level)    = 0;
    virtual void lowerINT(int level)    = 0;
    virtual void raiseNMI(void)         = 0;
    virtual void continueCPU(void)      = 0;
    virtual void systemMutexRelease()   = 0;
    virtual void systemMutexAcquire()   = 0;
    virtual void waitCPU()              = 0;

    virtual AddressBus& getAddressBus() = 0;
};

#endif // COMPUTER_H_
