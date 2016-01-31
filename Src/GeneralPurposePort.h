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

#include "config.h"
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
    virtual ~GeneralPurposePort();

    virtual BYTE in(BYTE addr);
    virtual void out(BYTE addr, BYTE val);

  private:

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

    static const BYTE Mtr89_Port174_Mask_c    = 0x03;   // Mask for determining controller
    // at right slot
    static const BYTE Mtr89_Port174_H_88_1_c  = 0x00;   // H-88-1 hard-sectored controller
    static const BYTE Mtr89_Port174_Z_89_47_c = 0x01;   // Z-89-47 8" floppy disk controller
    static const BYTE Mtr89_Port174_Undef1_c  = 0x02;   // Undefined
    static const BYTE Mtr89_Port174_Undef2_c  = 0x03;   // Undefined
    static const BYTE Mtr89_Port170_Mask_c    = 0x0c;   // Mask for determining controller
    // at left slot
    static const BYTE Mtr89_Port170_Unused_c  = 0x00;   // Undefined
    static const BYTE Mtr89_Port170_Z_89_47_c = 0x04;   // Z-89-47 8" floppy disk controller
    static const BYTE Mtr89_Port170_Undef1_c  = 0x08;   // Undefined
    static const BYTE Mtr89_Port170_Undef2_c  = 0x0c;   // Undefined
    static const BYTE Mtr89_PrimaryPort_c     = 0x10;   // Determine primary controller
    static const BYTE Mtr89_PrimaryPort_174_c = 0x10;   // Primary boot device is at port 174
    static const BYTE Mtr89_PrimaryPort_170_c = 0x00;   // Primary boot device is at port 170
    static const BYTE Mtr89_MemoryTest_Mask_c = Mtr88_MemoryTest_Mask_c;
    static const BYTE Mtr89_MemoryTest_On_c   = Mtr88_MemoryTest_On_c;
    static const BYTE Mtr89_MemoryTest_Off_c  = Mtr88_MemoryTest_Off_c;
    static const BYTE Mtr89_Baud_Mask_c       = 0x40;   // Baud mask
    static const BYTE Mtr89_Baud_9600_c       = Mtr88_Baud_9600_c;
    static const BYTE Mtr89_Baud_19200_c      = Mtr88_Baud_19200_c;
    static const BYTE Mtr89_AutoBoot_c        = 0x80;   // Autoboot on power-up

    /// MTR-90

    static const BYTE Mtr90_Port174_Mask_c    = Mtr89_Port174_Mask_c;
    static const BYTE Mtr90_Port174_H_88_1_c  = Mtr89_Port174_H_88_1_c;
    static const BYTE Mtr90_Port174_Z_89_47_c = Mtr89_Port174_Z_89_47_c;
    static const BYTE Mtr90_Port174_Z_89_67_c = 0x02;   // Z-89-67 Winchester and 8" floppy
    static const BYTE Mtr90_Port174_Undef_c   = Mtr89_Port174_Undef2_c;
    static const BYTE Mtr90_Port170_Mask_c    = Mtr89_Port170_Mask_c;
    static const BYTE Mtr90_Port170_Z_89_37_c = 0x00;   // Z-89-37 soft-sectored controller
    static const BYTE Mtr90_Port170_Z_89_47_c = 0x04;   // Z-89-47 8" floppy disk controller
    static const BYTE Mtr90_Port170_Z_89_67_c = 0x08;   // Z-89-67 Winchester and 8" floppy
    static const BYTE Mtr90_Port170_Undef_c   = Mtr89_Port170_Undef2_c;   // Undefined
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

    /// Enable the Timer Interrupt
    static const BYTE gpp_EnableTimer_c         = 0x02;

    /// Disable the ROM and map the RAM to the lower 8k.
    static const BYTE gpp_DisableROM_c          = 0x20;

    /// Side-select for the floppy drive (appears to be just for Hard-sectored controllers).
    static const BYTE gpp_SideSelect_c          = 0x40;

    /// Third party add-on for speed control
    static const BYTE gpp_4MHz_2MHz_Select_c    = 0x10;

    /// Magnolia Microsystems 128K RAM board.
    static const BYTE gpp_Mms_128k_Bit2_c       = 0x04;
    static const BYTE gpp_Mms_128k_Bit4_c       = 0x10;   // note: conflict with 2/4 MHz
    static const BYTE gpp_Mms_128k_Bit5_c       = 0x20;

    ///
    ///      The MMS 128K RAM board doubles as the upper 16K of a standard 64K memory
    /// system and as two 56K blocks of RAM accessible through block switching.  The
    /// 16K section is accessed normally, with no initialization necessary, but
    ///
    /// ----->  A KEY-LOCK SEQUENCE IS REQUIRED TO ACCESS THE OTHER 112K.   <-----
    /// ----->  THE KEY-LOCK SEQUENCE IS OUTPUT BY MAGNOLIA'S CP/M, SO IF   <-----
    /// ----->  YOU USE MMS CP/M, DO NOT BOTHER WITH THE KEY-LOCK SEQUENCE. <-----
    ///
    ///      A specific sequence of bytes (the Key-Lock sequence) must be output to
    /// port 0F2H before the block switching feature is activated (before any outputs
    /// to this port will affect the memory configuration).  The first time this
    /// sequence is output, no block switching will take place, but the block
    /// switching feature will be activated (further outputs to this port will change
    /// the memory configuration).  ONCE THE SEQUENCE HAS BEEN OUTPUT, REPEATING THE
    /// SEQUENCE WILL CHANGE THE MEMORY CONFIGURATION AS IT IS BEING OUTPUT, thus
    /// possibly switching the block holding the program outputting the sequence,
    /// with the result that something else (Code?  A data table?  'INVISIBLE DISK'
    /// files?) is executed.  The Key-Lock sequence may be output ONCE after the
    /// system is powered-up or reset without this occurring, and this is done by
    /// Magnolia Microsystems CP/M, so you don't have to bother if you are running
    /// MMS CP/M.  If you are running Zenith CP/M, you will have to do so.
    ///      If a program does not know whether the Key-Lock sequence has been
    /// output, it may safely output the sequence if it is in the top 8K of RAM
    /// (locations 0E000H to 0FFFFH) when it does so.  Repeatedly outputting the
    /// sequence will have no effect if the outputting is done by a program running
    /// in these locations.
    ///
    /// Key-Lock Sequence:
    ///       The Key-Lock sequence to be output to port 0F2H is:
    /// FIRST, DISABLE INTERRUPTS!!!!
    /// 00000100B     Note:  Leave the 2 millisecond clock off (bit 1 = 0) while
    /// 00001100B     this sequence is being output, and disable all interrupts.
    /// 00000100B     Interrupts vector through locations 0000H to 0038H, and those
    /// 00001000B     locations may not be valid when the interrupts occur if the
    /// 00001100B     memory is not in a standard configuration.
    /// 00001000B
    /// 00100010B  <---- Final byte sets up the system memory as 64K of RAM, and
    ///               turns on the 2 millisecond clock.
    /// Interrupts may be re-enabled after the sequence has been output.
    static const BYTE gpp_Mms_128k_Unlock_Mask_c   = 0x0c;
    static const BYTE gpp_Mms_128k_Unlock_Count_c  = 7;
    static const BYTE gpp_Mms_128k_Unlock_Seq_c[gpp_Mms_128k_Unlock_Count_c];
    bool mms128k_Unlocked;
    BYTE mms128k_Unlock_Pos;

    ///
    /// <pre>
    /// Memory Maps Available:
    ///
    ///      |                                               |
    ///      | - MPM Map: 16K common block - |               |
    ///      |               | - CPM Map:  8K common block - |
    ///      |               |               |               |
    /// 64K- +-------+-------+---------------+-------+-------+
    ///      |       |       |               |       |       |
    ///      |       |       |               |   0   |   0   |
    ///      |       |       |               |       |       |
    /// 56K- +   0   +   0   +               +-------+-------+
    ///      |       |       |               |       |       |
    ///      |       |       |               |       |       |
    ///      |       |       |               |       |       |
    /// 48K- +-------+-------+       0       +       +       +     +-------+-------+
    ///      |       |       |               |       |       |     |               |
    ///      |       |       |               |       |       |     |               |
    ///      ~       ~       ~               ~       ~       ~     ~   CPU board   ~
    ///          2       1                       1       2             RAM chips
    ///      ~       ~       ~               ~       ~       ~     ~     only      ~
    ///      |       |       |               |       |       |     |   (used for   |
    ///      |       |       |               |       |       |     |  maintenance) |
    ///  8K- +       +       +---+---+---+   +       +       +     +-------+       +
    ///      |       |       |   |   | R |   |       |       |     |       |       |
    ///      |       |       | 2 | 1 | O | 0 |       |       |     |  ROM  |       |
    ///      |       |       |   |   | M |   |       |       |     |       |       |
    ///  0K- +-------+-------+---+---+---+---+-------+-------+     +-------+-------+
    ///      |   F   |   E   | D | C | A | B |   G   |   H   |     |  Maintenance  |
    ///
    ///
    /// Port 0F2H Output Bit Patterns (bits are numbered from 0 to 7):
    ///  ('Byte' is if all other bits of the port are 0.  For example,
    ///   the 2 millisecond clock enable bit is bit 1, and map G with
    ///   the 2 millisecond clock on would be a byte of 15H.)
    ///
    ///         Bits
    /// Map  D5  D4  D2  Byte   Notes
    /// --------------------------------------------------------------
    ///  A   0   0   0   00H    (reset)
    ///  B   1   0   0   20H    (Std. CP/M)
    ///  C   0   0   1   04H    Use to initialize page 0 of maps E, G.
    ///  D   1   0   1   24H    Use to initialize page 0 of maps F, H.
    ///  E   0   1   0   10H
    ///  F   1   1   0   30H
    ///  G   0   1   1   14H
    ///  H   1   1   1   34H
    /// </pre>
    ///
    static const BYTE gpp_Mms_128k_Bank_Mask_c   = 0x34;
    static const BYTE gpp_Mms_128k_Bank_A_c      = 0x00;
    static const BYTE gpp_Mms_128k_Bank_B_c      = 0x20;
    static const BYTE gpp_Mms_128k_Bank_C_c      = 0x04;
    static const BYTE gpp_Mms_128k_Bank_D_c      = 0x24;
    static const BYTE gpp_Mms_128k_Bank_E_c      = 0x10;
    static const BYTE gpp_Mms_128k_Bank_F_c      = 0x30;
    static const BYTE gpp_Mms_128k_Bank_G_c      = 0x14;
    static const BYTE gpp_Mms_128k_Bank_H_c      = 0x34;

    int  curSide_m;
    bool fast_m;
};

#endif // GENERALPURPOSEPORT_H_
