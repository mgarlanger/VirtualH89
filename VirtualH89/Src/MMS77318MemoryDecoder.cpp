///
/// \file MMS77318MemoryDecoder.cpp
///
/// Implementation of the MMS 77318 128K add-on RAM card and
/// associated decoder modification.
///
/// \date Mar 05, 2016
/// \author Douglas Miller
///
#include "MMS77318MemoryDecoder.h"

#include "MemoryLayout.h"

#include "RAMemory8K.h"

#include "SystemMemory8K.h"

#include "logger.h"

using namespace std;


const BYTE MMS77318MemoryDecoder::lockSeq[6] = {
    0b00000100,
    0b00001100,
    0b00000100,
    0b00001000,
    0b00001100,
    0b00001000
};


MMS77318MemoryDecoder::MMS77318MemoryDecoder(systemMem_ptr systemMemory): MemoryDecoder(8,
                                                                                        h89_gppWatchedBits_c)
{
    int          x;

    Memory8K_ptr rd6[8];
    Memory8K_ptr rd7[8];

    for (x = 0; x < 8; x++)
    {
        rd6[x] = make_shared<RAMemory8K>(x << 13);
        rd7[x] = make_shared<RAMemory8K>(x << 13);
    }

    // 0: RESET state, ROM at 0x0000
    MemoryLayout_ptr h89_0 = make_shared<MemoryLayout>();
    h89_0->addPage(systemMemory);

    // add 48k of memory starting at 8k upto 56k
    for (x = 1; x < 7; x++)
    {
        h89_0->addPage(make_shared<RAMemory8K>(x << 13));
    }

    // save the 48k -> 56k bank for later
    shared_ptr<Memory8K>  ras2_1 = h89_0->getPage(6);
    h89_0->addPage(rd6[6]);
    h89_0->addPage(rd6[7]);

    addLayout(0, h89_0);

    // 1: CP/M state, 64K RAM
    MemoryLayout_ptr h89_1 = make_shared<MemoryLayout>();
    h89_1->addPageAt(ras2_1, 0x0000);
    for (x = 1; x < 8; ++x)
    {
        h89_1->addPage(h89_0->getPage(x));
    }
    addLayout(1, h89_1);

    // MMS 77318 additional layouts:
    // 2,3: Hi 56K always main, low 8K bank switched (HDOS)
    MemoryLayout_ptr hdos56k_1 = make_shared<MemoryLayout>();
    MemoryLayout_ptr hdos56k_2 = make_shared<MemoryLayout>();

    hdos56k_1->addPage(rd7[0]);
    hdos56k_2->addPage(rd6[0]);

    for (x = 1; x < 8; ++x)
    {
        hdos56k_1->addPage(h89_1->getPage(x));
        hdos56k_2->addPage(h89_1->getPage(x));
    }

    addLayout(2, hdos56k_1);
    addLayout(3, hdos56k_2);

    // 4,5: Hi 16K always main, low 48K bank switched
    MemoryLayout_ptr cpm64k_16k_1 = make_shared<MemoryLayout>();
    MemoryLayout_ptr cpm64k_16k_2 = make_shared<MemoryLayout>();
    for (x = 0; x < 6; ++x)
    {
        cpm64k_16k_1->addPage(rd7[x]);
        cpm64k_16k_2->addPage(rd6[x]);
    }
    for (; x < 8; ++x)
    {
        cpm64k_16k_1->addPage(h89_1->getPage(x));
        cpm64k_16k_2->addPage(h89_1->getPage(x));
    }

    addLayout(4, cpm64k_16k_1);
    addLayout(5, cpm64k_16k_2);

    // 6,7: Hi 8K always main, low 56K bank switched
    MemoryLayout_ptr cpm64k_8k_1 = make_shared<MemoryLayout>();
    MemoryLayout_ptr cpm64k_8k_2 = make_shared<MemoryLayout>();
    for (x = 0; x < 7; ++x)
    {
        cpm64k_8k_1->addPage(rd7[x]);
        cpm64k_8k_2->addPage(rd6[x]);
    }
    cpm64k_8k_1->addPage(rd7[6]);
    cpm64k_8k_1->addPage(h89_1->getPage(7));

    cpm64k_8k_2->addPageAt(rd7[7], 0xc000);
    cpm64k_8k_2->addPage(h89_1->getPage(7));

    addLayout(6, cpm64k_8k_1);
    addLayout(7, cpm64k_8k_2);

    lockState = 1;
    updateCurLayout(0);
}


MMS77318MemoryDecoder::~MMS77318MemoryDecoder()
{

}

void
MMS77318MemoryDecoder::reset()
{
    MemoryDecoder::reset();

    interestedBits_m = h89_gppWatchedBits_c;
    lockState        = 1;
}

void
MMS77318MemoryDecoder::gppNewValue(BYTE gpo)
{
    debugss(ssAddressBus, INFO, "MMS77318 gpio %02x lock state %d\n", gpo, lockState);
    int bnk = (gpo & h89_gppBnkSelBit0_c) ? 0x01 : 0; // LSB - a.k.a. GPIO5

    if (lockState == 0)
    {
        bnk |= (gpo & h89_gppBnkSelBit1_c) ? 0x02 : 0; // middle bit
        bnk |= (gpo & h89_gppBnkSelBit2_c) ? 0x04 : 0; // MSB
    }
    else
    {

        if ((gpo & h89_gppUnlockBits_c) == lockSeq[lockState])
        {
            ++lockState;
            debugss(ssAddressBus, INFO, "MMS77318 lock state %d\n", lockState);
            if (lockState >= sizeof(lockSeq))
            {
                debugss(ssAddressBus, ERROR, "MMS77318 unlocked\n");
                lockState        = 0;
                interestedBits_m = h89_gppBnkSelBits_c;
            }
        }
        else
        {
            lockState = 1;
        }
    }
    updateCurLayout(bnk);
}
