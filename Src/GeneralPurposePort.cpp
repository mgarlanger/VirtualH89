///
/// \file GeneralPurposePort.cpp
///
/// \date Apr 20, 2009
/// \author Mark Garlanger
///

#include "GeneralPurposePort.h"
#include "h89.h"
#include "logger.h"
#include "h89-timer.h"
#include "IODevice.h"

const BYTE GeneralPurposePort::gpp_Mms_128k_Unlock_Seq_c[gpp_Mms_128k_Unlock_Count_c] =
                                                     { 0x04,
    		                                           0x0c,
    		                                           0x04,
    		                                           0x08,
    		                                           0x0c,
    		                                           0x08,
    		                                           0x00
                                                     };

GeneralPurposePort::GeneralPurposePort(): IODevice(GPP_BaseAddress_c, GPP_NumPorts_c),
                                          mms128k_Unlocked(false),
                                          mms128k_Unlock_Pos(0),
                                          curSide_m(-1),
                                          fast_m(false)
{

}

GeneralPurposePort::~GeneralPurposePort()
{

}

BYTE GeneralPurposePort::in(BYTE addr)
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

    if (verifyPort(addr))
    {
        //return(Mtr89_MemoryTest_Off_c);

        return(Mtr89_MemoryTest_Off_c | Mtr89_Port170_Z_89_47_c);
    }

    return(0);
}

void GeneralPurposePort::out(BYTE addr, BYTE val)
{
    if (verifyPort(addr))
    {
        if (val & gpp_EnableTimer_c)
        {
            // enable timer interrupt.
            debugss(ssGpp, ALL, "%s: Enable Timer Interrupt.\n", __FUNCTION__);
            h89.getTimer().enableINT();
        }
        else
        {
            debugss(ssGpp, ALL, "%s: Disable Timer Interrupt.\n", __FUNCTION__);
            h89.getTimer().disableINT();
        }

        if (val & gpp_DisableROM_c)
        {
            // ORG "0" Mod - disable the ROM and use RAM for the lower 8K
            debugss(ssGpp, ALL, "%s: Enable ORG 0.\n", __FUNCTION__);
            h89.disableROM();
        }
        else
        {
            // re-enable the ROM
            debugss(ssGpp, ALL, "%s: Disable ORG 0.\n", __FUNCTION__);
            h89.enableROM();
        }

        if (val & gpp_SingleStepInterrupt_c)
        {
            debugss(ssGpp, WARNING, "%s: Single Step Interrupt - not implemented.\n",
                    __FUNCTION__);
            /// \todo - implement single step interrupt..
        }

        if (val & gpp_SideSelect_c)
        {
            debugss(ssGpp, ALL, "%s: H17 Set Side 1.\n", __FUNCTION__);
            h89.selectSideH17(1);
        }
        else
        {
            debugss(ssGpp, ALL, "%s: H17 Set Side 0.\n", __FUNCTION__);
        	h89.selectSideH17(0);
        }

        /// Speed changes, may need to change to support MMS 128k expansion.
        if (val & gpp_4MHz_2MHz_Select_c)
        {
        	/// \todo this needs to be put in the H89 class...
        	if (!fast_m)
        	{
                debugss(ssGpp, ALL, "%s: Set Fast speed.\n", __FUNCTION__);
        	    h89.setSpeed(true);
        	    fast_m = true;
        	}
        }
        else
        {
        	if (fast_m)
        	{
                debugss(ssGpp, ALL, "%s: Set Standard speed.\n", __FUNCTION__);
        	    h89.setSpeed(false);
        	    fast_m = false;
        	}
        }
    }
}
