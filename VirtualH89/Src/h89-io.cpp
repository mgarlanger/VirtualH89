/// \file h89-io.cpp
///
/// \date Mar 7, 2009
/// \author Mark Garlanger
///
/// \brief Implements framework to handle all the I/O devices on the H89.
///

#include "h89-io.h"

#include "logger.h"
#include "DiskController.h"


H89_IO::H89_IO()
{
    debugss(ssIO, INFO, "%\n");

    for (int port = 0; port < 256; ++port)
    {
        iodevices[port] = 0;
    }
}

H89_IO::~H89_IO()
{
    debugss(ssIO, INFO, "\n");
}

std::vector<DiskController*>&
H89_IO::getDiskDevices()
{
    return dsk_devs;
}
bool
H89_IO::addDiskDevice(DiskController* device)
{
    dsk_devs.push_back(device);
    return addDevice(device);
}

bool
H89_IO::addDevice(IODevice* device)
{
    debugss(ssIO, INFO, "\n");

    if (device == NULL)
    {
        // NULL device passed in
        debugss(ssIO, ERROR, "Null Device\n");
        return (false);
    }

    BYTE base = device->getBaseAddress();
    BYTE num  = device->getNumPorts();
    BYTE last = base + num;

    debugss(ssIO, INFO, "ports (%03o - %03o)\n", base, last);

    if (!num)
    {
        // no ports.
        debugss(ssIO, ERROR, "no ports\n");

        return (false);
    }

    // First make sure there is no conflict
    for (BYTE port = base; port < last; ++port)
    {
        if (iodevices[port])
        {
            // Address already in use
            debugss(ssIO, ERROR, "duplicate devices on port (%03o)\n", port);
            return (false);
        }
    }

    // Now set the new value
    for (BYTE port = base; port < last; ++port)
    {
        iodevices[port] = device;
    }

    return (true);
}

bool
H89_IO::removeDevice(IODevice* device)
{
    bool retVal = true;

    if (device)
    {
        BYTE base = device->getBaseAddress();
        BYTE num  = device->getNumPorts();
        BYTE last = base + num;

        debugss(ssIO, INFO, "ports (%03o - %03o)\n", base, last);

        if (num)
        {
            for (BYTE port = base; port < last; ++port)
            {
                if (iodevices[port] == device)
                {
                    // TODO: call destructor? (i.e. "delete iodevices[port];"?)
                    iodevices[port] = 0;
                }
                else
                {
                    // Doesn't match what is attempting to be removed.
                    debugss(ssIO, ERROR, "non-matching device on port (%03o)\n", port);
                    retVal = false;
                }
            }
        }
        else
        {
            // no ports.
            debugss(ssIO, ERROR, "no ports\n");

            retVal = false;
        }
    }
    else
    {
        // NULL device passed in
        debugss(ssIO, ERROR, "Null Device\n");

        retVal = false;
    }

    return (retVal);
}

void
H89_IO::reset()
{
    for (int port = 0; port < 256; ++port)
    {
        if (iodevices[port] != NULL)
        {
            iodevices[port]->reset();
        }
    }
}

BYTE
H89_IO::in(BYTE addr)
{
    BYTE val = 0xff;

    debugss(ssIO, ALL, "(%03o)\n", addr);

    if (iodevices[addr])
    {
        val = iodevices[addr]->in(addr);
    }
    else
    {
        // undefined in
        debugss(ssIO, WARNING, "undefined port (%03o)\n", addr);
    }

    debugss(ssIO, ALL, "(%03o): %d\n", addr, val);
    return (val);
}

void
H89_IO::out(BYTE addr,
            BYTE val)
{
    debugss(ssIO, ALL, "(%03o) = 0x%02x\n", addr, val);

    if (iodevices[addr])
    {
        iodevices[addr]->out(addr, val);
    }
    else
    {
        debugss(ssIO, WARNING, "undefined port (%03o) = 0x%02x\n", addr, val);
    }
}
