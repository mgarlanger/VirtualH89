///
///  \file IMDFloppyDisk.cpp
///
///  \author Mark Garlanger
///  \date April 17, 2016
///

#include "IMDFloppyDisk.h"

#include "Track.h"
#include "Sector.h"

#include "GenericFloppyFormat.h"
#include "logger.h"


#include <iostream>
#include <fstream>
#include <cstdlib>


GenericFloppyDisk*
IMDFloppyDisk::getDiskette(std::vector<std::string> argv)
{
    GenericFloppyDisk* gd = new IMDFloppyDisk(argv);

    if (!gd->isReady())
    {
        delete gd;
        gd = nullptr;
    }

    return gd;
}


IMDFloppyDisk::IMDFloppyDisk(std::vector<std::string> argv): GenericFloppyDisk(),
                                                             imageName_m(nullptr),
                                                             curSector_m(nullptr),
                                                             dataPos_m(0),
                                                             sectorLength_m(0),
                                                             secLenCode_m(0),
                                                             hypoTrack_m(false),
                                                             hyperTrack_m(false),
                                                             ready_m(true)
{
    if (argv.size() < 1)
    {
        debugss(ssFloppyDisk, WARNING, "no file specified\n");
        return;
    }

    char* name = strdup(argv[0].c_str());
    debugss(ssFloppyDisk, INFO, "reading: %s\n", name);

    for (int x = 1; x < argv.size(); ++x)
    {
        if (argv[x].compare("rw") == 0)
        {
            writeProtect_m = false;
        }
    }


    if (!readIMD(name))
    {
        ready_m = false;
        debugss(ssFloppyDisk, ERROR, "Read of file %s failed\n", name);
    }
    
    free(name);
}

IMDFloppyDisk::~IMDFloppyDisk()
{

}

bool
IMDFloppyDisk::findSector(int side,
                          int track,
                          int sector)
{

    if (!tracks_m[side][track])
    {
        return false;
    }
    curSector_m = tracks_m[side][track]->findSector(sector);
    if (!curSector_m)
    {
        return false;
    }
    if (curSector_m->getHeadNum() != side || curSector_m->getTrackNum() != track)
    {
        curSector_m = nullptr;
        return false;
    }

    sectorLength_m = curSector_m->getSectorLength();
    // TODO handle if the L options is set to '0'.
    switch (sectorLength_m)
    {
        case 128:
            secLenCode_m = 0;
            break;

        case 256:
            secLenCode_m = 1;
            break;

        case 512:
            secLenCode_m = 2;
            break;

        case 1024:
            secLenCode_m = 3;
            break;

        default:
            debugss(ssFloppyDisk, ERROR, "bad sector size: %s\n", sectorLength_m);
            curSector_m = nullptr;
            return false;
    }

    return true;
}


bool
IMDFloppyDisk::readIMD(const char* name)
{
    std::ifstream     file;
    unsigned long int fileSize;
    unsigned long int pos = 0;

    BYTE*             buf;

    debugss(ssFloppyDisk, INFO, "file: %s\n", name);

    file.open(name, std::ios::binary);
    file.seekg(0, std::ios::end);
    fileSize = file.tellg();
    file.seekg(0, std::ios::beg);

    buf      = new BYTE[fileSize];
    file.read((char*) buf, fileSize);
    file.close();

    // get past the EOF character.
    while (buf[pos++] != 0x1a)
    {
        if (pos == fileSize)
        {
            delete [] buf;

            return (false);
        }
    }

    BYTE            sectorOrder[256];
    BYTE            sectorCylMapping[256];
    BYTE            sectorHeadMapping[256];
    unsigned char   mode, cyl, head, numSec, sectorSizeKey;
    int             sectorSize;
    bool            sectorCylMap, sectorHeadMap;
    Track::Density  density;
    Track::DataRate dataRate;

    // After the text comment at the beginning, the rest of the file is individual tracks.
    do
    {
        // mode
        mode = buf[pos++];

        switch (mode)
        {
            case 0:
                density  = Track::singleDensity;
                dataRate = Track::dr_500kbps;
                debugss(ssFloppyDisk, INFO, "FM/500 kbps\n");
                break;

            case 1:
                density  = Track::singleDensity;
                dataRate = Track::dr_300kbps;
                debugss(ssFloppyDisk, INFO, "FM/300 kbps\n");
                break;

            case 2:
                density  = Track::singleDensity;
                dataRate = Track::dr_250kbps;
                debugss(ssFloppyDisk, INFO, "FM/250 kbps\n");
                break;

            case 3:
                density  = Track::doubleDensity;
                dataRate = Track::dr_500kbps;
                debugss(ssFloppyDisk, INFO, "MFM/500 kbps\n");
                break;

            case 4:
                density  = Track::doubleDensity;
                dataRate = Track::dr_300kbps;
                debugss(ssFloppyDisk, INFO, "MFM/300 kbps\n");
                break;

            case 5:
                density  = Track::doubleDensity;
                dataRate = Track::dr_250kbps;
                debugss(ssFloppyDisk, INFO, "MFM/250 kbps\n");
                break;

            default:
                density  = Track::density_Unknown;
                dataRate = Track::dr_Unknown;
                debugss(ssFloppyDisk, WARNING, "Unknown density/data rate: %d\n", mode);
                break;

        }
        doubleDensity_m = (density == Track::doubleDensity);

        cyl             = buf[pos++];
        debugss(ssFloppyDisk, ALL, "Cylinder:    %d\n", cyl);

        head            = buf[pos++];

        // Check flags.
        sectorCylMap    = ((head & 0x80) != 0);
        sectorHeadMap   = ((head & 0x40) != 0);

        // mask off any flags
        head           &= 1;
        debugss(ssFloppyDisk, ALL, "Head:    %d\n", head);

        Track* trk = new Track(head, cyl);

        numSec        = buf[pos++];
        debugss(ssFloppyDisk, ALL, "Num Sectors:    %d\n", numSec);

        sectorSizeKey = buf[pos++];

        if (sectorSizeKey < 7)
        {
            sectorSize = 1 << (sectorSizeKey + 7);
        }
        else
        {
            debugss(ssFloppyDisk, ERROR, "Sector Size unknown: %d\n", sectorSizeKey);
            delete [] buf;
            delete trk;

            return (false);
        }

        debugss(ssFloppyDisk, ALL, "Sector Size: %d (%d)\n", sectorSize, sectorSizeKey);

        for (int i = 0; i < numSec; i++)
        {
            sectorOrder[i] = buf[pos++];
        }

        if (sectorCylMap)
        {
            debugss(ssFloppyDisk, INFO, "Sector Cylinder Map Present\n");

            for (int i = 0; i < numSec; i++)
            {
                sectorCylMapping[i] = buf[pos++];
            }
        }

        if (sectorHeadMap)
        {
            debugss(ssFloppyDisk, INFO, "Sector Head Map Present\n");

            for (int i = 0; i < numSec; i++)
            {
                sectorHeadMapping[i] = buf[pos++];
            }
        }

        for (int i = 0; i < numSec; i++)
        {
            unsigned char sectorType = buf[pos++];

            debugss(ssFloppyDisk, INFO, "Sector: %d(%d) Type: %d\n", sectorOrder[i], i, sectorType);

            if (sectorType == 0x00)
            {
                // Sector Data Unavailable
                debugss(ssFloppyDisk, INFO, "Sector Size unknown: %d\n", sectorSizeKey);
            }
            else if (sectorType > 0x08)
            {
                // Out of Range.
                debugss(ssFloppyDisk, ERROR, "Out of Range: %d\n", sectorType);
                delete [] buf;

                return false;
            }
            else
            {
                // by subtracting 1, can get the value to act like a bit mask.
                BYTE type       = sectorType - 1;

                bool compressed = ((type & 0x01) != 0);
                bool deleteData = ((type & 0x02) != 0);
                bool dataError  = ((type & 0x04) != 0);
                BYTE sectorData[sectorSize];

                if (compressed)
                {
                    BYTE val = buf[pos++];

                    for (int i = 0; i < sectorSize; i++)
                    {
                        sectorData[i] = val;
                    }
                }
                else
                {
                    for (int i = 0; i < sectorSize; i++)
                    {
                        sectorData[i] = buf[pos++];
                    }
                }

                // create sector
                Sector* sect = new Sector(head, cyl, sectorOrder[i], sectorSize, sectorData);

                // set flags
                sect->setReadError(dataError);
                sect->setDeletedDataAddressMark(deleteData);

                // add sector to track
                trk->addSector(sect);
            }
        }

        // add track to disk
        tracks_m[head].push_back(trk);

    }
    while (pos < fileSize);

    delete [] buf;

    debugss(ssFloppyDisk, INFO, "Read successful.\n");

    return true;
}


bool
IMDFloppyDisk::readData(BYTE track,
                        BYTE side,
                        BYTE sector,
                        int  inSector,
                        int& data)
{

    if (inSector < 0)
    {
        if (sector == 0xfd)
        {
            data = GenericFloppyFormat::ID_AM;
            return true;
        }
        else if (sector == 0xff)
        {
            data = GenericFloppyFormat::INDEX_AM;
            return true;
        }
        else if (findSector(side, track, sector))
        {
            dataPos_m = 0;
            data      = GenericFloppyFormat::DATA_AM;
            return true;
        }

        data = GenericFloppyFormat::NO_DATA;
        return true;
    }

    if (sector == 0xfd)
    {
        switch (inSector)
        {
            case 0:
                data = track;
                break;

            case 1:
                data = side;
                break;

            case 2:
                data = 1; // anything will do? 'sector' is 0xfd...
                break;

            case 3:
                data = secLenCode_m;
                break;

            case 4:
                data = 0; // CRC 1
                break;

            case 5:
                data = 0; // CRC 2
                break;

            default:
                data = GenericFloppyFormat::CRC;
                break;
        }

        return true;
    }

    else if (sector == 0xff)
    {
        if (inSector < sectorLength_m)
        {
            // TODO: implement this
            data = 0;
        }
        else
        {
            data = GenericFloppyFormat::CRC;
        }

        return true;
    }

    if (dataPos_m < sectorLength_m)
    {
        if (curSector_m)
        {
            BYTE sectorData;
            curSector_m->readData(dataPos_m++, sectorData);
            data = sectorData;
        }
        else
        {
            debugss(ssFloppyDisk, ERROR, "curSector_m not set\n");
        }
    }
    else
    {
        debugss(ssFloppyDisk, INFO, "data done %d %d %d\n", track, side, sector);
        data = GenericFloppyFormat::CRC;
    }

    return true;
}

bool
IMDFloppyDisk::writeData(BYTE track,
                         BYTE side,
                         BYTE sector,
                         int  inSector,
                         BYTE data,
                         bool dataReady,
                         int& result)
{

    if (checkWriteProtect())
    {
        debugss(ssSectorFloppyImage, ERROR, "write protect\n");
        return false;
    }

    if (inSector < 0)
    {
        if (sector == 0xff || sector == 0xfe)
        {
            result = GenericFloppyFormat::ERROR;
            return true;
        }
        else if (findSector(side, track, sector))
        {
            dataPos_m = 0;
            result    = GenericFloppyFormat::DATA_AM;
            return true;
        }

        result = GenericFloppyFormat::NO_DATA;
        return true;
    }

    debugss(ssSectorFloppyImage, INFO, "pos=%d data=%02x\n", inSector, data);

    if (dataPos_m < sectorLength_m)
    {

        if (!dataReady)
        {
            debugss(ssSectorFloppyImage, ERROR, "data not read pos=%d\n", inSector);
            result = GenericFloppyFormat::NO_DATA;
        }
        else
        {
            if (curSector_m)
            {
                curSector_m->writeData(dataPos_m++, data);

                result = data;
            }
            else
            {
                debugss(ssFloppyDisk, ERROR, "curSector_m not set\n");
            }
        }
    }
    else
    {
        debugss(ssSectorFloppyImage, INFO, "CRC pos=%d data=%02x\n", inSector, data);
        result = GenericFloppyFormat::CRC;
    }

    return true;
}

BYTE
IMDFloppyDisk::getMaxSectors(BYTE side,
                             BYTE track)
{
    if (!tracks_m[side][track])
    {
        return 0;
    }

    return tracks_m[side][track]->getMaxSectors();
}
bool
IMDFloppyDisk::isReady()
{
    return ready_m;
}

void
IMDFloppyDisk::eject(const char* name)
{

}

void
IMDFloppyDisk::dump(void)
{

}

std::string
IMDFloppyDisk::getMediaName()
{
    if (!imageName_m)
    {
        return "NONE";
    }

    return imageName_m;
}
