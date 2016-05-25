///
/// \name Sector.cpp
///
///
/// \date Jun 22, 2013
/// \author Mark Garlanger
///

#include "Sector.h"

#include "logger.h"

/// \cond
#include <iostream>
/// \endcond


Sector::Sector(BYTE  headNum,
               BYTE  trackNum,
               BYTE  sectorNum,
               WORD  sectorLength,
               BYTE* data): headNum_m(headNum),
                            trackNum_m(trackNum),
                            sectorNum_m(sectorNum),
                            deletedDataAddressMark_m(false),
                            readError_m(false),
                            valid_m(false),
                            sectorLength_m(sectorLength)
{
    data_m = new BYTE[sectorLength_m];

    if (data_m)
    {
        valid_m = true;

        for (int i = 0; i < sectorLength_m; i++)
        {
            data_m[i] = data[i];
        }
    }
}

Sector::Sector(BYTE headNum,
               BYTE trackNum,
               BYTE sectorNum,
               WORD sectorLength,
               BYTE data): headNum_m(headNum),
                           trackNum_m(trackNum),
                           sectorNum_m(sectorNum),
                           deletedDataAddressMark_m(false),
                           readError_m(false),
                           valid_m(false),
                           sectorLength_m(sectorLength)
{
    data_m = new BYTE[sectorLength_m];

    if (data_m)
    {
        valid_m = true;

        for (int i = 0; i < sectorLength_m; i++)
        {
            data_m[i] = data;
        }
    }
}

Sector::~Sector()
{
    if (data_m)
    {
        delete[] data_m;
    }

    valid_m = false;
}


void
Sector::setDeletedDataAddressMark(bool val)
{
    deletedDataAddressMark_m = val;
}

void
Sector::setReadError(bool val)
{
    readError_m = val;
}

bool
Sector::getDeletedDataAddressMark()
{
    return deletedDataAddressMark_m;
}
bool
Sector::getReadError()
{
    return readError_m;
}

void
Sector::dump()
{
    debugss(ssFloppyDisk, INFO, "Sector dump - Side: %d Track: %d Sector: %d\n",
            headNum_m, trackNum_m, sectorNum_m);

    char printable[17];

    printable[16] = 0;

    for (int i = 0; i < sectorLength_m; i++)
    {
        if ((i % 16) == 0)
        {
            debugss(ssFloppyDisk, INFO, "     0x%04x: ", i);
        }
        else
        {
            if ((i % 8) == 0)
            {
                debugss_nts(ssFloppyDisk, INFO, " ");
            }
        }

        // store printable char for end of line
        printable[i % 16] = (isprint(data_m[i])) ? data_m[i] : '.';

        // print hex value
        debugss_nts(ssFloppyDisk, INFO, "%02x", data_m[i]);

        if (((i + 1) % 16) == 0)
        {
            debugss_nts(ssFloppyDisk, INFO, "  %s\n", printable);
        }
    }
}

BYTE
Sector::getSectorNum()
{
    return sectorNum_m;
}

WORD
Sector::getSectorLength()
{
    return sectorLength_m;
}

BYTE
Sector::getHeadNum()
{
    return headNum_m;
}
BYTE
Sector::getTrackNum()
{
    return trackNum_m;
}

bool
Sector::readData(WORD  pos,
                 BYTE& data)
{
    debugss(ssFloppyDisk, INFO, "pos: %d\n", pos);

    if (pos < sectorLength_m)
    {
        data = data_m[pos];

        debugss(ssFloppyDisk, INFO, "Valid pos: %d 0x%02x\n", data, data);

        return true;
    }

    return false;
}

bool
Sector::writeData(WORD pos,
                  BYTE data)
{
    debugss(ssFloppyDisk, INFO, "pos: %d\n", pos);

    if (pos < sectorLength_m)
    {
        data_m[pos] = data;

        debugss(ssFloppyDisk, INFO, "Valid pos: %d 0x%02x\n", pos, data);

        return true;
    }
    debugss(ssFloppyDisk, ERROR, "pos: %d sectorLength_m: %d\n", pos, sectorLength_m);

    return false;
}
