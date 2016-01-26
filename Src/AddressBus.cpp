/// \file AddressBus.cpp
///
///  \date Mar 7, 2009
///  \author Mark Garlanger
///

#include "logger.h"
#include "AddressBus.h"
#include "Memory.h"

/// \todo  - allow both RAM and ROM in the first 8k and support reading ROM and writing to
///         RAM at the same time.

AddressBus::AddressBus()
{
    debugss(ssAddressBus, INFO, "%s\n", __FUNCTION__);

    for (int i = 0; i < numOfPages_c; i++)
    {
        // empty memory.
        mem[i] = 0;
    }
}

AddressBus::~AddressBus()
{
    debugss(ssAddressBus, INFO, "%s\n", __FUNCTION__);
}

BYTE AddressBus::readByte(WORD addr)
{
    BYTE index = addr >> pageSizeBits_c;

    debugss(ssAddressBus, ALL, "%s: addr(%d), index(%d)\n", __FUNCTION__, addr, index);

    if (mem[index])
    {
        return (mem[index]->readByte(addr));
    }
    else
    {
        // read from non-existent memory.
        // Based on a comment from Terry Gulczynski, on a H89A, U521 will put a 0x00 byte on
    	// the data bus whenever you attempt to read from non-existent memory.

        debugss(ssAddressBus, INFO, "%s: Non-existent memory\n", __FUNCTION__);
        return 0x00;
    }
}

void AddressBus::writeByte(WORD addr, BYTE val)
{
    BYTE index = addr >> pageSizeBits_c;

    debugss(ssAddressBus, ALL, "%s: addr(%d) = %d, index(%d)\n", __FUNCTION__,
            addr, val, index);

    if (mem[index])
    {
        mem[index]->writeByte(addr, val);
    }
    else
    {
        // otherwise just drop the byte...
        debugss(ssAddressBus, INFO, "%s: Non-existent memory addr(%d) = %d\n",
                __FUNCTION__, addr, val);
    }
}

void AddressBus::installMemory(Memory *memory)
{
    WORD base = memory->getBaseAddress();
    int  size = memory->getSize();

    debugss(ssAddressBus, INFO, "%s: Base = %d  size = %d", __FUNCTION__, base, size);

    BYTE index = base >> pageSizeBits_c;
    BYTE numIndex = (size >> pageSizeBits_c);

    debugss(ssAddressBus, VERBOSE, "    index = %d   numIndex = %d\n", index, numIndex);

    for (int i = 0; i < numIndex; i++)
    {
        int pos = (index + i) % numOfPages_c;
        debugss(ssAddressBus, VERBOSE, "%s: mem[%d] = 0x%llx", __FUNCTION__, pos,
        		(unsigned long long int) memory);

        if (mem[pos])
        {
        	/// \todo with current design this is not an error, the swap between ROM and RAM
        	///       will hit this condition. Need to change the design to make sure it
        	///       behaves like a real H89 (i.e. write when the ROM and selected and it
        	///       will write to RAM).

            debugss(ssAddressBus, VERBOSE, "%s: overriding , orig base: 0x%x orig size: %d\n",
                    __FUNCTION__, mem[pos]->getBaseAddress(), mem[pos]->getSize());
            debugss(ssAddressBus, VERBOSE, "%s: Base = %d  size = %d", __FUNCTION__, base,
                    size);
        }
        mem[pos] = memory;
    }
}

void AddressBus::clearMemory(BYTE data)
{
    debugss(ssAddressBus, INFO, "%s: data(%d)", __FUNCTION__, data);

    for (int i = 0; i < 0x10000; i++)
    {
        writeByte(i, data);
    }
}
