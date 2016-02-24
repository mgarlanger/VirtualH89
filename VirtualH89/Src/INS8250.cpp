/// \file INS8250.cpp
///
/// \date Apr 4, 2009
/// \author Mark Garlanger
///


#include "INS8250.h"

#include "H89.h"
#include "WallClock.h"
#include "logger.h"

#include "SerialPortDevice.h"


INS8250::INS8250(BYTE base, int intLevel): IODevice(base, 8),
                                           DLAB_m(false),
                                           ERBFI_m(false),
                                           receiveInterruptPending(false),
                                           OE_m(false),
                                           PE_m(false),
                                           FE_m(false),
                                           device_m(0),
                                           rxByteAvail(false),
                                           txByteAvail(false),
                                           lsBaudDiv(0),
                                           msBaudDiv(0),
                                           baud_m(0),
                                           lastTransmit(0),
                                           saveIER(0),
                                           saveIIR(0),
                                           saveLCR(0),
                                           saveMCR(0),
                                           saveLSR(0),
                                           saveMSR(MSB_ClearToSend | MSB_DataSetReady)

{
    intLevel_m = intLevel;

    /// \todo Use H89 manual to verify/set all the conditions on reset of chip.
}

INS8250::~INS8250()
{

}

void
INS8250::reset()
{
    DLAB_m                  = false;
    ERBFI_m                 = false;
    receiveInterruptPending = false;
    OE_m                    = false;
    PE_m                    = false;
    FE_m                    = false;
    rxByteAvail             = false;
    txByteAvail             = false;
    lsBaudDiv               = 0;
    msBaudDiv               = 0;
    baud_m                  = 0;
    lastTransmit            = 0;
    saveIER                 = 0;
    saveIIR                 = 0;
    saveLCR                 = 0;
    saveMCR                 = 0;
    saveLSR                 = 0;
    saveMSR                 = MSB_ClearToSend | MSB_DataSetReady;
}

BYTE
INS8250::in(BYTE addr)
{
    BYTE val = 0x00;

    if (verifyPort(addr))
    {
        BYTE offset = addr - baseAddress_m;

        switch (offset)
        {
            case RBR: // Receiver Buffer Register
                if (DLAB_m)
                {
                    // set baud rate
                    // LS Byte
                    val = lsBaudDiv;
                }

                else
                {
                    if (rxByteAvail)
                    {
                        rxByteAvail             = false;
                        receiveInterruptPending = false;
                        lowerInterrupt();
                        val                     = RecvBuf;
                    }
                }

                break;

            case IER: // Interrupt Enable Register
                if (DLAB_m)
                {
                    // set baud rate
                    // LS Byte
                    val = msBaudDiv;
                }

                else
                {
                    val = saveIER;
                }

                break;

            case IIR: // Interrupt Identification Register
                if (receiveInterruptPending)
                {
                    val = IIR_DataAvailable;
                }

                else
                {
                    val = IIR_NoInterruptPending;
                }

                break;

            case LCR: // Line Control Register
                val = saveLCR;
                break;

            case MCR: // Modem Control Register
                val = saveMCR;
                break;

            case LSR: // Line Status Register
                if (rxByteAvail)
                {
                    val |= LSB_DataReady;
                }

                if ((WallClock::instance()->getClock() - lastTransmit) > 2133)
                {
                    val |= LSB_THRE;
                }

                if (1) /// \todo - what to do here?
                {
                    val |= LSB_TSRE;
                }

                if (OE_m)
                {
                    val |= LSB_Overrun;
                    OE_m = false;
                }

                if (FE_m)
                {
                    val |= LSB_FramingError;
                    FE_m = false;
                }

                if (PE_m)
                {
                    val |= LSB_ParityError;
                    PE_m = false;
                }

                break;

            case MSR: // Modem Status Register
                val = saveMSR;
                break;

            default:
                /// \todo determine  what value is set here.
                return (0);

        }

        debugss(ss8250, INFO, "%s: %d(%d) <- %d", __FUNCTION__, addr, offset, val);
    }

    else
    {
        debugss(ss8250, ERROR, "%s: Verify Port failed: %d", __FUNCTION__, addr);
        return (0);
    }

    // debug("%d\n", val);
    return (val);
}

void
INS8250::out(BYTE addr, BYTE val)
{
    if (verifyPort(addr))
    {
        BYTE offset = addr - baseAddress_m;

        switch (offset)
        {
            case THR:
                if (!DLAB_m)
                {

                    if (device_m)
                    {
                        device_m->receiveData(val);
                        lastTransmit = WallClock::instance()->getClock();
                    }

                    else
                    {
                        debugss(ss8250, ERROR, "%s: THR - No device_m.", __FUNCTION__);
                    }
                }

                else
                {
                    unsigned int div;
                    lsBaudDiv = val;
                    div       = (msBaudDiv << 8) | (lsBaudDiv);

                    if (div)
                    {
                        baud_m = 115200 / div;
                    }

                    debugss(ss8250, INFO, "baud_m = %d\n", baud_m);
                }

                break;

            case IER:
                if (!DLAB_m)
                {
                    saveIER = val;

                    if (val & IER_ReceiveData)
                    {
                        ERBFI_m = true;
                    }

                    else
                    {
                        ERBFI_m = false;
                    }

                }

                else
                {
                    unsigned int div;
                    // set baud.
                    msBaudDiv = val;
                    div       = (msBaudDiv << 8) | (lsBaudDiv);

                    if (div)
                    {
                        baud_m = 115200 / div;
                    }

                    debugss(ss8250, INFO, "baud_m = %d\n", baud_m);
                }

                break;

            case IIR:
                saveIIR = val;
                break;

            case LCR:
                saveLCR        = val;
                bits_m         = (val & 0x3) + 5;
                stopBits_m     = (val & 0x4);
                parityEnable_m = (val & 0x8);
                evenParity_m   = (val & 0x10);
                stickParity_m  = (val & 0x20);
                break_m        = (val & 0x40);
                DLAB_m         = (val & 0x80);
                break;

            case MCR:
                saveMCR = val;
                break;

            case LSR:
                saveLSR = val;
                break;

            case MSR:
                saveMSR = val;
                break;

            default:
                // nothing to save.
                break;
        }
    }
}


bool
INS8250::attachDevice(SerialPortDevice* dev)
{
    if (!device_m)
    {
        device_m = dev;
        device_m->attachPort(this);
        return true;
    }

    return false;
}


bool
INS8250::receiveReady()
{
    return !rxByteAvail;
}

void
INS8250::receiveData(BYTE data)
{
    debugss(ss8250, ALL, "%s - %d\n", __FUNCTION__, data);

    unsigned int baud = device_m->getBaudRate();

    if (baud == SerialPortDevice::DISABLE_BAUD_CHECK)
    {
        baud = baud_m;
    }

    if (baud == baud_m)
    {
        debugss(ss8250, ALL, "%s: Baud matches\n", __FUNCTION__);
        PE_m = false;
        FE_m = false;
    }

    else if (baud > baud_m)
    {
        // computer is at lower baud than the remote device.
        debugss(ss8250, ALL, "%s: device baud exceeds our baud.\n", __FUNCTION__);
        PE_m = true;
        FE_m = false;
        // a full character will not be received, so we must exit and not set rxByteAvail
        return;
    }

    else
    {
        // computer is at a faster baud than the remote device.
        debugss(ss8250, ALL, "%s: device baud lower\n", __FUNCTION__);
        PE_m = false;
        FE_m = true;
    }

    RecvBuf     = data;
    rxByteAvail = true;

    if (ERBFI_m)
    {
        receiveInterruptPending = true;

        raiseInterrupt();
    }
}

/// \todo many more areas in this file that can raise and lower the interrupt.
void
INS8250::raiseInterrupt()
{
    if (intLevel_m >= 0)
    {
        h89.raiseINT(intLevel_m);
    }

}


void
INS8250::lowerInterrupt()
{
    if (intLevel_m >= 0)
    {
        h89.lowerINT(intLevel_m);
    }

}
