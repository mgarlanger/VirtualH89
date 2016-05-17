///
/// \file GeneralPurposePort.cpp
///
/// \date Apr 20, 2009
/// \author Mark Garlanger
///

#include "GeneralPurposePort.h"

#include <stdlib.h>

#include "logger.h"
#include "propertyutil.h"
#include "GppListener.h"
#include "computer.h"

using namespace std;

GeneralPurposePort::GeneralPurposePort(Computer* computer): IODevice(GPP_BaseAddress_c,
                                                                     GPP_NumPorts_c),
                                                            portBits_m(0),
                                                            computer_m(computer)
{
    dipsw_m = (Mtr89_MemoryTest_Off_c | Mtr89_Port170_Z_89_47_c);

}

GeneralPurposePort::GeneralPurposePort(Computer*   computer,
                                       string      settings): IODevice(GPP_BaseAddress_c,
                                                                       GPP_NumPorts_c),
                                                              portBits_m(0),
                                                              computer_m(computer)
{
    // TODO: verify a binary string and/or handle other formats/nmenonics.
    dipsw_m = strtol(settings.c_str(), nullptr, 2);
}

GeneralPurposePort::~GeneralPurposePort()
{

}

void
GeneralPurposePort::reset()
{
    // do not change 'dispsw_m'!
    // portBits_m = 0; // must actually call out()... side-effects...
    out(GPP_BaseAddress_c, 0);
}

BYTE
GeneralPurposePort::in(BYTE addr)
{
    /// The general purpose port returns the value of SW501.
    /// This varies depending on the monitor ROM.
    ///
    /// MTR-88
    ///
    /// Only bits 5-8 are defined for the MTR-88
    ///
    /// bits 4-0 - Undefined.
    ///
    /// bit 5 - force the memory test.
    ///   0 - normal operation
    ///   1 - memory test
    ///
    /// bit 7 & 6 define the baud rate
    ///  00 -   9600
    ///  01 - 19,200
    ///  10 - 38,400
    ///  11 - 57,600
    ///

    ///
    /// MTR-89
    ///
    /// bits 1 & 0 - Controller in Port 0174(0x7c)/0177(0x7f)
    ///
    ///  00 - H-88-1
    ///  01 - H/Z-47 controller
    ///  10 - Undefined
    ///  11 - Undefined
    ///
    /// bits 3 & 2 - Controller in Port 0170(0x78)/0173(0x7b)
    ///
    ///  00 - Not in use.
    ///  01 - H/Z-47 controller
    ///  10 - Undefined
    ///  11 - Undefined
    ///
    /// bit 4 - Primary boot device
    ///
    ///  0 - Port 0174/0177 (H-88-1)
    ///  1 - Port 0170/0173 (H/Z-47)
    ///
    /// bit 5 - Memory Test
    ///
    ///  0 - Perform Memory test upon boot-up
    ///  1 - Normal operation
    ///
    /// bit 6 - Console Baud rate
    ///
    ///  0 - 9600
    ///  1 - 19200
    ///
    /// bit 7 - Auto boot.
    ///
    ///  0 - Normal (no auto boot)
    ///  1 - Auto boot.
    ///

    ///
    /// MTR-90
    ///
    ///  Same as MTR-89 except for bits 0-3
    ///
    /// bits 1 & 0 - Controller in Port 0174(0x7c)/0177(0x7f)
    ///
    ///  00 - H-88-1
    ///  01 - Z-89-47
    ///  10 - Z-89-67
    ///  11 - Undefined
    ///
    /// bits 3 & 2 - Controller in Port 0170(0x78)/0173(0x7b)
    ///
    ///  00 - Z-89-37
    ///  01 - Z-89-47
    ///  10 - Z-89-67
    ///  11 - Undefined
    ///
    BYTE val = 0;

    if (verifyPort(addr))
    {
        val = dipsw_m;
    }

    return val;
}

void
GeneralPurposePort::out(BYTE addr,
                        BYTE val)
{
    if (verifyPort(addr))
    {
        // from the manual, writing to this port clears the interrupt.
        computer_m->lowerINT(1);

        BYTE diffs = portBits_m ^ val;

        portBits_m = val;

        if (val & gpp_SingleStepInterrupt_c)
        {
            debugss(ssGpp, WARNING, "Single Step Interrupt - not implemented.\n");
            /// \todo - implement single step interrupt..
        }

        if (diffs != 0)
        {
            GppListener::notifyListeners(val, diffs);
        }
    }
}

string
GeneralPurposePort::dumpDebug()
{
    string ret = PropertyUtil::sprintf("GP-OUT=%02x GP-IN=%02x\n", portBits_m, dipsw_m);

    return ret;
}
