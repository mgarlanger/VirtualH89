///
/// \name Sector.cpp
///
///
/// \date Jun 22, 2013
/// \author Mark Garlanger
///

#include "Sector.h"

#include "logger.h"

#include <iostream>

Sector::Sector(): headNum_m(0),
                  trackNum_m(0),
                  sectorNum_m(0),
                  deletedDataAddressMark_m(false),
                  readError_m(false),
                  valid_m(false),
                  data_m(0),
                  sectorLength_m(0)
{

}

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
Sector::initialize(BYTE  headNum,
                   BYTE  trackNum,
                   BYTE  sectorNum,
                   WORD  sectorLength,
                   BYTE* data)
{
    if (valid_m)
    {
        // Error condition, shouldn't initialize an already valid object

        debugss(ssFloppyDisk, ERROR, "%s: Sector already initialized - h: %d t: %d s: %d\n",
                __FUNCTION__, headNum, trackNum, sectorNum);

    }

    headNum_m      = headNum;
    trackNum_m     = trackNum;
    sectorNum_m    = sectorNum;
    sectorLength_m = sectorLength;
    data_m         = new BYTE[sectorLength_m];

    if (data_m)
    {
        valid_m = true;

        for (int i = 0; i < sectorLength_m; i++)
        {
            data_m[i] = data[i];
        }
    }
    else
    {
        valid_m = false;
    }
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

void
Sector::dump()
{
    printf("Sector dump - Side: %d Track: %d Sector: %d\n", headNum_m,
           trackNum_m,
           sectorNum_m);

    char printable[17];

    printable[16] = 0;

    for (int i = 0; i < sectorLength_m; i++)
    {
        if ((i % 16) == 0)
        {
            printf("     0x%04x: ", i);
        }
        else
        {
            if ((i % 8) == 0)
            {
                printf(" ");
            }
        }

        // store printable char for end of line
        printable[i % 16] = (isprint(data_m[i])) ? data_m[i] : '.';

        // print hex value
        printf("%02x", data_m[i]);

        if (((i + 1) % 16) == 0)
        {
            printf("  %s\n", printable);
        }
    }
}

BYTE
Sector::getSectorNum()
{
    return sectorNum_m;
}

bool
Sector::readData(WORD pos, BYTE& data)
{
    debugss(ssFloppyDisk, INFO, "%s: pos: %d\n", __FUNCTION__, pos);

    if (pos < sectorLength_m)
    {
        data = data_m[pos];

        debugss(ssFloppyDisk, INFO, "%s: Valid pos: %d 0x%02x\n", __FUNCTION__, data, data);

        return true;
    }

    return false;
}

bool
Sector::writeData(WORD pos, BYTE data)
{
    debugss(ssFloppyDisk, INFO, "%s: pos: %d\n", __FUNCTION__, pos);

    if (pos < sectorLength_m)
    {
        data = data_m[pos];

        debugss(ssFloppyDisk, INFO, "%s: Valid pos: %d 0x%02x\n", __FUNCTION__, data, data);

        return true;
    }

    return false;
}
