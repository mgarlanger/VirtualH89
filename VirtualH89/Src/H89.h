///
/// \file H89.h
///
/// \date Mar 8, 2009
/// \author Mark Garlanger
///

/// \mainpage Virtual Heathkit H89 All-In-One Computer
///
/// The projects implements a virtual Heathkit H-89 Computer. Heath Company also
/// offered the same system without the floppy drive/controller as the H-88. The computer was
/// also sold under the Zenith Data Systems (ZDS) brand as the Z-89 and Z-90.
///
/// This system was sold by Heath Company and ZDS between 1979 and 1984.
///
/// Website for emulator: http://heathkit.garlanger.com/
///
#ifndef H89_H_
#define H89_H_

#include "config.h"
#include "computer.h"
#include "cpu.h"
#include "Console.h"

// Forward declare classes to avoid a tangled mess of includes.

class ROM;
class RAM;
class AddressBus;
class InterruptController;
class H89Timer;
class H89_IO;
class NMIPort;
class GeneralPurposePort;
class INS8250;
class H17;
class Z_89_37;
class Z47Interface;
class Z47Controller;
class DiskDrive;
class CPU;
class Terminal;
class FloppyDisk;
class ParallelLink;

///
/// \brief Virtual Heathkit %H89 Computer
///
/// Implements all the pieces of the H89 Computer.
///
class H89 : public Computer
{
  private:
    AddressBus          *ab;
    InterruptController *interruptController;
    H89Timer            *timer;

    H89_IO              *h89io;

    GeneralPurposePort  *gpp;

    NMIPort             *nmi1;
    NMIPort             *nmi2;

    INS8250             *consolePort;
    INS8250             *lpPort, *modemPort, *auxPort;

    H17                 *h17;
    Console             *console;
    Z_89_37             *h37;
    Z47Interface        *z47If;
    Z47Controller       *z47Cntrl;
    ParallelLink        *z47Link;

    DiskDrive           *driveUnitH0;
    DiskDrive           *driveUnitH1;
    DiskDrive           *driveUnitH2;

    DiskDrive           *driveUnitS0;
    DiskDrive           *driveUnitS1;
    DiskDrive           *driveUnitS2;
    DiskDrive           *driveUnitS3;

    DiskDrive           *driveUnitE0;
    DiskDrive           *driveUnitE1;

    FloppyDisk          *hard0;
    FloppyDisk          *hard1;
    FloppyDisk          *hard2;

    FloppyDisk          *soft0;
    FloppyDisk          *soft1;
    FloppyDisk          *soft2;
    FloppyDisk          *soft3;

    FloppyDisk          *eight0;
    FloppyDisk          *eight1;

    CPU                 *cpu;

    ROM                 *monitorROM;
    ROM                 *h17ROM;
    RAM                 *memory;

    /// \todo consolidate the RAM into one and change how ORG-0 is handled. - maybe not.
    /// \todo also need to change the way this is handled so writes when rom enabled, go to
    ///       the RAM.
    RAM                 *h17RAM;
    RAM                 *CPM8k;

    /// Port Addresses

    /// Base address for the H17 controller
    /// Octal 174
    static const BYTE H17_BaseAddress_c    = 0x7c;  // Octal 174

    /// Possible base addresses for the H37 controller
    /// Octal 174
    static const BYTE H37_BasePort_c                = 0x78;

    static const BYTE Z47_BasePort1_c               = 0x78;
    static const BYTE Z47_BasePort2_c               = 0x7c;


    /// Addresses for the serial ports
    static const BYTE Serial_Console_c           = 0350; // (0xe8)
    static const BYTE Serial_Console_Interrupt_c = 3;
    static const BYTE Serial_LpPort_c            = 0320;
    static const BYTE Serial_ModemPort_c         = 0330;
    static const BYTE Serial_AuxPort_c           = 0340;

    /// Address for NMI
    /// Octal 360-361 and 372-373
    static const BYTE NMI_BaseAddress_1_c = 0xf0;
    static const BYTE NMI_NumPorts_1_c    = 2;
    static const BYTE NMI_BaseAddress_2_c = 0xfa;
    static const BYTE NMI_NumPorts_2_c    = 2;

    /// Address for GPP (General Purpose Port)
    /// Octal 362
    static const BYTE GPP_BaseAddress_c   = 0xf2;

    /// Frequency 2.048 MHz
    static const unsigned long cpuClockRate_c            = 2048000;
    /// 2 mSec Interrupt
    static const unsigned int  clockInterruptPerSecond_c = 500;

  public:
    H89();
    virtual ~H89();
    void buildSystem(Console *console);

    virtual void reset();
    virtual BYTE run();
    virtual H89Timer& getTimer();

    virtual void init();

    virtual void keypress(BYTE ch);
    virtual void display();

    virtual void registerInter(CPU::intrCheck *func, void *data);
    virtual void unregisterInter(CPU::intrCheck *func);
    virtual void assertBUSREQ();
    virtual void deassertBUSREQ();
    virtual void raiseINT(int level);
    virtual void lowerINT(int level);
    virtual void raiseNMI(void);
    virtual void continueCPU(void);
    virtual void waitCPU(void);

    virtual void disableROM();
    virtual void enableROM();

    virtual void writeProtectH17RAM();
    virtual void writeEnableH17RAM();

    virtual void selectSideH17(BYTE side);

    virtual void setSpeed(bool fast);

    virtual H89_IO& getIO();

    virtual void clearMemory(BYTE data = 0);

    virtual AddressBus& getAddressBus();
};

extern H89 h89;

#endif // H89_H_
