/// \file GeneralPurposePort.h
///
///  The H89 has this 'General Purpose Port', that allows the CPU to read the configured
///  dip switches on the CPU port. It is also used to control some behavior when written.
///
/// \date Apr 20, 2009
/// \author Mark Garlanger
///

#ifndef GENERALPURPOSEPORT_H_
#define GENERALPURPOSEPORT_H_

#include <string>

#include "IODevice.h"

///
/// \brief General Purpose Port
///
/// Reads from the General Purpose Port, returns the values of a dip switch on the CPU board.
/// Writes to the General Purpose Port, control some behavior such as timer interrupts,
/// mapping of ROM, and single-step.
///
class GeneralPurposePort: public virtual IODevice
{
  public:
    GeneralPurposePort();
    GeneralPurposePort(std::string settings);
    virtual ~GeneralPurposePort();

    virtual BYTE in(BYTE addr);
    virtual void out(BYTE addr,
                     BYTE val);

    virtual std::string dumpDebug();
    void reset();

  private:
    BYTE              dipsw_m;
    BYTE              portBits_m;

    /// Address for General Purpose Port
    /// Octal 362
    static const BYTE GPP_BaseAddress_c = 0xf2;
    static const BYTE GPP_NumPorts_c    = 1;

    /// Configuration Values

    /// MTR-88

    /// Memory Test at power up
    static const BYTE Mtr88_MemoryTest_Mask_c = 0x20;
    /// Memory Test at power up on
    static const BYTE Mtr88_MemoryTest_On_c   = 0x00;
    /// Memory Test at power up off
    static const BYTE Mtr88_MemoryTest_Off_c  = 0x20;
    /// Baud rate Mask
    static const BYTE Mtr88_Baud_Mask_c       = 0xc0;
    /// Baud rate 9600
    static const BYTE Mtr88_Baud_9600_c       = 0x00;
    /// Baud rate 19200
    static const BYTE Mtr88_Baud_19200_c      = 0x40;
    /// Baud rate 38400
    static const BYTE Mtr88_Baud_38400_c      = 0x80;
    /// Baud rate 57600
    static const BYTE Mtr88_Baud_57600_c      = 0xc0;

    /// MTR-89

    static const BYTE Mtr89_Port174_Mask_c    = 0x03; // Mask for determining controller
    // at right slot
    static const BYTE Mtr89_Port174_H_88_1_c  = 0x00; // H-88-1 hard-sectored controller
    static const BYTE Mtr89_Port174_Z_89_47_c = 0x01; // Z-89-47 8" floppy disk controller
    static const BYTE Mtr89_Port174_Undef1_c  = 0x02; // Undefined
    static const BYTE Mtr89_Port174_Undef2_c  = 0x03; // Undefined
    static const BYTE Mtr89_Port170_Mask_c    = 0x0c; // Mask for determining controller
    // at left slot
    static const BYTE Mtr89_Port170_Unused_c  = 0x00; // Undefined
    static const BYTE Mtr89_Port170_Z_89_47_c = 0x04; // Z-89-47 8" floppy disk controller
    static const BYTE Mtr89_Port170_Undef1_c  = 0x08; // Undefined
    static const BYTE Mtr89_Port170_Undef2_c  = 0x0c; // Undefined
    static const BYTE Mtr89_PrimaryPort_c     = 0x10; // Determine primary controller
    static const BYTE Mtr89_PrimaryPort_174_c = 0x00; // Primary boot device is at port 174
    static const BYTE Mtr89_PrimaryPort_170_c = 0x10; // Primary boot device is at port 170
    static const BYTE Mtr89_MemoryTest_Mask_c = Mtr88_MemoryTest_Mask_c;
    static const BYTE Mtr89_MemoryTest_On_c   = Mtr88_MemoryTest_On_c;
    static const BYTE Mtr89_MemoryTest_Off_c  = Mtr88_MemoryTest_Off_c;
    static const BYTE Mtr89_Baud_Mask_c       = 0x40; // Baud mask
    static const BYTE Mtr89_Baud_9600_c       = Mtr88_Baud_9600_c;
    static const BYTE Mtr89_Baud_19200_c      = Mtr88_Baud_19200_c;
    static const BYTE Mtr89_AutoBoot_c        = 0x80; // Autoboot on power-up

    /// MTR-90

    static const BYTE Mtr90_Port174_Mask_c    = Mtr89_Port174_Mask_c;
    static const BYTE Mtr90_Port174_H_88_1_c  = Mtr89_Port174_H_88_1_c;
    static const BYTE Mtr90_Port174_Z_89_47_c = Mtr89_Port174_Z_89_47_c;
    static const BYTE Mtr90_Port174_Z_89_67_c = 0x02; // Z-89-67 Winchester and 8" floppy
    static const BYTE Mtr90_Port174_Undef_c   = Mtr89_Port174_Undef2_c;
    static const BYTE Mtr90_Port170_Mask_c    = Mtr89_Port170_Mask_c;
    static const BYTE Mtr90_Port170_Z_89_37_c = 0x00;                   // Z-89-37 soft-sectored controller
    static const BYTE Mtr90_Port170_Z_89_47_c = 0x04;                   // Z-89-47 8" floppy disk controller
    static const BYTE Mtr90_Port170_Z_89_67_c = 0x08;                   // Z-89-67 Winchester and 8" floppy
    static const BYTE Mtr90_Port170_Undef_c   = Mtr89_Port170_Undef2_c; // Undefined
    static const BYTE Mtr90_PrimaryPort_174_c = Mtr89_PrimaryPort_174_c;
    static const BYTE Mtr90_PrimaryPort_170_c = Mtr89_PrimaryPort_170_c;
    static const BYTE Mtr90_MemoryTest_Mask_c = Mtr89_MemoryTest_Mask_c;
    static const BYTE Mtr90_MemoryTest_On_c   = Mtr89_MemoryTest_On_c;
    static const BYTE Mtr90_MemoryTest_Off_c  = Mtr89_MemoryTest_Off_c;
    static const BYTE Mtr90_Baud_Mask_c       = Mtr89_Baud_Mask_c;
    static const BYTE Mtr90_Baud_9600_c       = Mtr89_Baud_9600_c;
    static const BYTE Mtr90_Baud_19200_c      = Mtr89_Baud_19200_c;
    static const BYTE Mtr90_AutoBoot_c        = Mtr89_AutoBoot_c;

    /// Out constants
    /// Enable Single Step Interrupt - not sure about this.
    static const BYTE gpp_SingleStepInterrupt_c = 0x01;

};

#endif // GENERALPURPOSEPORT_H_
