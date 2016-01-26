/// \file INS8250.h
///
/// \brief Virtual 8250 Serial UART
///
/// \date Apr 4, 2009
/// \author Mark Garlanger
///

#ifndef INS8250_H_
#define INS8250_H_

#include "h89Types.h"
#include "IODevice.h"

class SerialPortDevice;

///
/// \brief Serial Port
///
/// 8250 Serial Port
///
class INS8250 : public IODevice
{
  public:
    INS8250(BYTE baseAddr, int IntLevel = -1);
    virtual ~INS8250();

    virtual BYTE in(BYTE addr);
    virtual void out(BYTE addr, BYTE val);

    virtual bool attachDevice(SerialPortDevice *dev);

    virtual void receiveData(BYTE data);

    // TODO - add all the status, both for the device to set it's status
    //        and for the port to set the status.

  private:
    int intLevel_m;

    /// Line Control variables:
    bool DLAB_m;    // Divisor Latch Access bit
    BYTE bits_m;
    bool stopBits_m;
    bool parityEnable_m;
    bool evenParity_m;
    bool stickParity_m;
    bool break_m;
    bool ERBFI_m;
    bool receiveInterruptPending;

    // Modem Control variables

    /// Data Terminal Ready
    bool DTR_m;

    /// Request to Send
    bool RTS_m;
    bool OUT1_m;
    bool OUT2_m;

    /// Loopback
    bool Loop_m;

    // Line Status variables

    /// Data Ready
    bool DR_m;

    /// Overrun Error
    bool OE_m;

    /// Parity Error
    bool PE_m;

    /// Framing Error
    bool FE_m;

    /// Break Interrupt
    bool BI_m;

    /// Transmitter Holding Register Empty
    bool THRE_m;

    /// Transmitter Shift Register Empty
    bool TSRE_m;

    SerialPortDevice *device_m;

    bool rxByteAvail;
    BYTE RecvBuf;
    bool txByteAvail;
    BYTE TransHolding;

    BYTE lsBaudDiv;
    BYTE msBaudDiv;

    WORD baud_m;

    unsigned long lastTransmit;

    /// Interrupt Enable Register
    BYTE saveIER;

    /// Interrupt Identification Register
    BYTE saveIIR;

    /// Line Control Register
    BYTE saveLCR;

    /// Modem Control Register
    BYTE saveMCR;

    /// Line Status Register
    BYTE saveLSR;

    /// Modem Status Register
    BYTE saveMSR;

    ///
    /// Register offsets from the base address.
    ///
    enum RegisterOffsets
    {
        /// Receiver Buffer Register (Read Only)
        RBR = 0,
        /// Transmitter Holding Register (Write Only)
        THR = 0,
        /// Divisor Latch (Low - LS)
        DLL = 0,
        /// Interrupt Enable Register
        IER = 1,
        /// Divisor Latch (High - MS)
        DLH = 1,
        /// Interrupt Identification Register
        IIR = 2,
        /// Line Control Register
        LCR = 3,
        /// Modem Control Register
        MCR = 4,
        /// Line Status Register
        LSR = 5,
        /// Modem Status Register
        MSR = 6,
        /// SCratch Register
        SCR = 7
    };

    ///
    /// Interrupt Enable Register Bits
    ///

    /// Interrupt Enable Register - Receive Data
    static const BYTE IER_ReceiveData          = 0x01;

    /// Interrupt Enable Register - Transmit Holding Register Empty
    static const BYTE IER_TransmitHoldingEmpty = 0x02;

    /// Interrupt Enable Register - Line Status
    static const BYTE IER_LineStatus           = 0x04;

    /// Interrupt Enable Register - Receive Data
    static const BYTE IER_ModemStatus          = 0x08;

    //
    // Interrupt Identification Register
    //

    /// Interrupt Identification Register - No Interrupt Pending
    static const BYTE IIR_NoInterruptPending   = 0x01;

    /// Interrupt Identification Register - Line Status
    static const BYTE IIR_LineStatus           = 0x06;

    /// Interrupt Identification Register - Data Available
    static const BYTE IIR_DataAvailable        = 0x04;

    /// Interrupt Identification Register - Transmitter Empty
    static const BYTE IIR_TransmitterEmpty     = 0x02;

    /// Interrupt Identification Register - Modem Status
    static const BYTE IIR_ModemStatus          = 0x00;

    //
    // Line Control Register Bits
    //

    /// Line Control Register - Word Length
    static const BYTE LCR_WordLength           = 0x03;

    /// Line Control Register - Stop Bits
    static const BYTE LCR_NumberOfStopBits     = 0x04;

    /// Line Control Register - Parity Enable
    static const BYTE LCR_ParityEnable         = 0x08;

    /// Line Control Register - Even Parity
    static const BYTE LCR_EvenParitySelect     = 0x10;

    /// Line Control Register - Stick Parity
    static const BYTE LCR_StickParity          = 0x20;

    /// Line Control Register - Set Break
    static const BYTE LCR_SetBreak             = 0x40;

    /// Line Control Register - Divisor Latch Access
    static const BYTE LCR_DivisorLatchAccess   = 0x80;

    //
    // Line Status Register Bits
    //

    ///  Line Status Register - Data Ready
    static const BYTE LSB_DataReady            = 0x01;

    /// Line Status Register - Overrun
    static const BYTE LSB_Overrun              = 0x02;

    /// Line Status Register - Parity Error
    static const BYTE LSB_ParityError          = 0x04;

    /// Line Status Register - Framing Error
    static const BYTE LSB_FramingError         = 0x08;

    /// Line Status Register - Break Interrupt
    static const BYTE LSB_BreakInterrupt       = 0x10;

    /// Line Status Register - Transmitting Holding Register Empty
    static const BYTE LSB_THRE                 = 0x20;

    /// Line Status Register - Transmitting Shift Register Empty
    static const BYTE LSB_TSRE                 = 0x40;

    //
    // Modem Status Register Bits
    //

    /// Modem Status Register - Delta Clear to Send
    static const BYTE MSB_DeltaClearToSend             = 0x01;

    /// Modem Status Register - Delta Data Set Ready
    static const BYTE MSB_DeltaDataSetReady            = 0x02;

    /// Modem Status Register - Trailing Edge Ring Indicator
    static const BYTE MSB_TrailingEdgeRI               = 0x04;

    /// Modem Status Register - Delta Receive Line Signal Detect
    static const BYTE MSB_DeltaReceiveLineSignalDetect = 0x08;

    /// Modem Status Register - Clear to Send
    static const BYTE MSB_ClearToSend                  = 0x10;

    /// Modem Status Register - Data Set Ready
    static const BYTE MSB_DataSetReady                 = 0x20;

    /// Modem Status Register - Ring Indicator
    static const BYTE MSB_RingIndicator                = 0x40;

    /// Modem Status Register - Received Line Signal Detect
    static const BYTE MSB_ReceivedLineSignalDetect     = 0x80;
};

#endif // INS8250_H_
