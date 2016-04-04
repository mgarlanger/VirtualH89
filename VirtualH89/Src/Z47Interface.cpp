///
/// \name Z47Interface.cpp
///
/// Interface card on the H89 to support the Z47 external dual 8" floppy disks.
///
/// \date Jul 14, 2013
/// \author Mark Garlanger
///

#include "Z47Interface.h"

#include "logger.h"
#include "ParallelLink.h"

Z47Interface::Z47Interface(int baseAddr): DiskController(baseAddr, H47_NumPorts_c),
                                          interruptsEnabled_m(false),
                                          DTR_m(false),
                                          DDOut_m(false),
                                          Busy_m(true),
                                          Error_m(false),
                                          Done_m(false),
                                          linkToDrive_m(0)
{
    debugss(ssH47, ALL, "(%d)\n", baseAddr);

}

Z47Interface::~Z47Interface()
{

}

BYTE
Z47Interface::in(BYTE addr)
{
    debugss(ssH47, ALL, "(%d)\n", addr);
    BYTE offset = getPortOffset(addr);
    BYTE data   = 0;

    switch (offset)
    {
        case StatusPort_Offset_c:
            readStatus(data);
            debugss(ssH47, ALL, "StatusPort - 0x%02x\n", data);
            break;

        case DataPort_Offset_c:
            readData(data);
            debugss(ssH47, ALL, "DataPort - 0x%02x\n", data);
            break;

        default:
            debugss(ssH47, ERROR, "Unknown Port offset - %d\n", offset);
            break;
    }

    return data;
}

void
Z47Interface::out(BYTE addr,
                  BYTE val)
{
    BYTE offset = getPortOffset(addr);

    debugss(ssH47, ALL, "(%d) = 0x%02x\n", addr, val);

    switch (offset)
    {
        case StatusPort_Offset_c:
            debugss(ssH47, ALL, "StatusPort\n");
            writeStatus(val);
            break;

        case DataPort_Offset_c:
            debugss(ssH47, ALL, " DataPort\n");
            writeData(val);
            break;

        default:
            debugss(ssH47, ERROR, "Unknown Port offset - %d\n", offset);
            break;
    }
}


void
Z47Interface::reset(void)
{
    debugss(ssH47, ALL, "\n");
    /// \todo Determine what to do for a reset.

    if (linkToDrive_m)
    {
        linkToDrive_m->masterReset();
    }
}

void
Z47Interface::notification(unsigned int cycleCount)
{
    debugss(ssH47, ALL, "\n");

}

void
Z47Interface::writeStatus(BYTE cmd)
{
    debugss(ssH47, ALL, "cmd = 0x%02x\n", cmd);

    if ((cmd & cmd_MasterReset_c) == cmd_MasterReset_c)
    {
        debugss(ssH47, INFO, "Master Reset\n");
        reset();
    }

    if ((cmd & cmd_U137BSet_c) == cmd_U137BSet_c)
    {
        debugss(ssH47, INFO, "FlipFlop U137B\n");
    }

    if ((cmd & cmd_InterruptsEnabled_c) == cmd_InterruptsEnabled_c)
    {
        debugss(ssH47, INFO, "Interrupts Enabled\n");
        interruptsEnabled_m = true;
    }
    else
    {
        interruptsEnabled_m = false;
    }

    if ((cmd & cmd_Undefined_c) != 0)
    {
        debugss(ssH47, WARNING, "Unexpected command bits: 0x%02x\n", cmd);
    }
}

void
Z47Interface::writeData(BYTE data)
{
    debugss(ssH47, ALL, "data = 0x%02x\n", data);

    if (linkToDrive_m)
    {
        linkToDrive_m->sendDriveData(data);
    }
    else
    {
        debugss(ssH47, ERROR, "link to Drive not configured\n");
    }
}


void
Z47Interface::readStatus(BYTE& data)
{
    BYTE val = 0;

    if (interruptsEnabled_m)
    {
        val |= stat_InterruptEnabled_c;
    }

    if (DTR_m)
    {
        val |= stat_DataTransferRequest_c;
    }

    if (Done_m)
    {
        val |= stat_Done_c;
    }

    data = val;

    debugss(ssH47, ALL, "Status 0x%02x\n", data);
}

void
Z47Interface::readData(BYTE& data)
{
    if (linkToDrive_m)
    {
        linkToDrive_m->readDataBusByHost(data);
        linkToDrive_m->setDTAK(true);
    }

    debugss(ssH47, ALL, "Data 0x%02x\n", data);
}


void
Z47Interface::connectDriveLink(ParallelLink* link)
{
    debugss(ssH47, ALL, "\n");

    linkToDrive_m = link;

    if (linkToDrive_m)
    {
        linkToDrive_m->registerHost(this);
    }
    else
    {
        debugss(ssH47, ERROR, "link to Drive not configured\n");
    }
}

void
Z47Interface::raiseSignal(SignalType sigType)
{
    debugss(ssH47, ALL, "\n");

    switch (sigType)
    {
        // Master Reset (Host to Disk System)
        case st_MasterReset:
            // Handle a reset
            debugss(ssH47, ERROR, "Invalid Master Reset received.\n");
            break;

        // Data Acknowledge (Host to Disk System)
        case st_DTAK:
            // Handle data acknowledge.
            debugss(ssH47, ERROR, "Invalid DTAK received.\n");
            break;

        // Data Ready (Disk System to Host)
        case st_DTR:
            debugss(ssH47, INFO, "DTR raised.\n");
            DTR_m = true;
            break;

        // Disk Drive Out (Disk System to Host)
        case st_DDOUT:
            debugss(ssH47, INFO, "DDOUT raised.\n");
            DDOut_m = true;
            break;

        // Busy (Disk System to Host)
        case st_Busy:
            debugss(ssH47, INFO, "Busy raised.\n");
            Busy_m = true;
            Done_m = false;
            break;

        // Error (Disk System to Host)
        case st_Error:
            debugss(ssH47, INFO, "Error raised.\n");
            Error_m = true;
            break;

        default:
            debugss(ssH47, ERROR, "Invalid sigType received.\n");
            break;
    }
}

void
Z47Interface::lowerSignal(SignalType sigType)
{
    debugss(ssH47, ALL, "\n");

    switch (sigType)
    {
        // Master Reset (Host to Disk System)
        case st_MasterReset:
            // Handle a reset
            debugss(ssH47, ERROR, "Invalid Master Reset received.\n");
            break;

        // Data Acknowledge (Host to Disk System)
        case st_DTAK:
            debugss(ssH47, ERROR, "Invalid DTAK received.\n");
            break;

        // Data Ready (Disk System to Host)
        case st_DTR:
            debugss(ssH47, INFO, "DTR lowered.\n");
            DTR_m = false;
            linkToDrive_m->setDTAK(false);

            break;

        // Disk Drive Out (Disk System to Host)
        case st_DDOUT:
            debugss(ssH47, INFO, "DDOUT lowered.\n");
            DDOut_m = false;
            break;

        // Busy (Disk System to Host)
        case st_Busy:
            debugss(ssH47, INFO, "Busy lowered.\n");
            Busy_m = false;
            Done_m = true;
            break;

        // Error (Disk System to Host)
        case st_Error:
            debugss(ssH47, INFO, "Error lowered.\n");
            Error_m = false;
            break;

        default:
            debugss(ssH47, ERROR, "Invalid sigType received.\n");
            break;
    }
}

void
Z47Interface::pulseSignal(SignalType sigType)
{
    debugss(ssH47, ALL, "\n");
}
