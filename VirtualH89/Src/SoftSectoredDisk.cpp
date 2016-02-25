///
/// \name SoftSectoredDisk.cpp
///
///
/// \date Oct 2, 2012
/// \author Mark Garlanger
///

#include "SoftSectoredDisk.h"

#include "logger.h"
#include "Track.h"
#include "Sector.h"

#include <fstream>
#include <strings.h>

SoftSectoredDisk::SoftSectoredDisk(const char* name,
                                   DiskImageFormat format): initialized_m(false)
{
    debugss(ssFloppyDisk, INFO, "%s: Insert Disk: %s\n", __FUNCTION__, name);

    if (format == dif_Unknown)
    {
        determineDiskFormat(name, format);
    }

    switch (format)
    {
        case dif_IMD:
            readIMD(name);
            break;

        case dif_TD0:
            readTD0(name);
            break;

        case dif_RAW:
            readRaw(name);
            break;

        case dif_8RAW:
            readRaw8(name);
            break;

        default:
            // Unknown format
            debugss(ssFloppyDisk, ERROR, "%s: Unknown disk format: %d\n", __FUNCTION__, format);

            break;
    }
}

SoftSectoredDisk::SoftSectoredDisk()
{
    debugss(ssFloppyDisk, INFO, "%s: Insert Empty Disk\n", __FUNCTION__);

    // clear disk
    bzero(rawImage_m, maxHeads_c * bytesPerTrack_c * tracksPerSide_c);

    numTracks_m   = tracksPerSide_c;
    numHeads_m    = maxHeads_c;
    maxTrack_m    = tracksPerSide_c;

    initialized_m = true;
    setWriteProtect(false);
}

SoftSectoredDisk::~SoftSectoredDisk()
{

}

bool
SoftSectoredDisk::readData(BYTE side,
                           BYTE track,
                           unsigned int pos,
                           BYTE& data)
{
    debugss(ssFloppyDisk, ALL, "%s: maxTrack (%d) tracks_m(%d)\n", __FUNCTION__, maxTrack_m,
            numTracks_m);

    // Currently only 40 and 80 are supported.
    if ((maxTrack_m != 40) && (maxTrack_m != 80))
    {
        debugss(ssFloppyDisk, ERROR, "%s: Invalid maxTrack: %d\n", __FUNCTION__, maxTrack_m);
        return false;
    }

    if (maxTrack_m != numTracks_m)
    {
        if ((maxTrack_m == 80) && (numTracks_m = 40))
        {
            debugss(ssFloppyDisk, INFO, "%s: max - 80 trk_m - 40\n", __FUNCTION__);
            track /= 2;
        }

        else if ((maxTrack_m == 40) && (numTracks_m == 80))
        {
            debugss(ssFloppyDisk, INFO, "%s: max - 40 trk_m - 80\n", __FUNCTION__);
            track *= 2;
        }
    }

    if (initialized_m)
    {
        if ((track < tracksPerSide_c) && (pos < bytesPerTrack_c))
        {
            debugss(ssFloppyDisk, ALL, "%s: track(%d) pos(%d) = %d\n", __FUNCTION__,
                    track, pos, rawImage_m[side][track][pos]);
            data = rawImage_m[side][track][pos];
            return true;
        }

        else
        {
            debugss(ssFloppyDisk, ERROR, "%s: range error: track(%d) pos(%d)\n", __FUNCTION__,
                    track, pos);
            return false;
        }
    }

    else
    {
        debugss(ssFloppyDisk, ERROR, "%s: disk not initialized\n", __FUNCTION__);
        return false;
    }
}

bool
SoftSectoredDisk::writeData(BYTE side,
                            BYTE track,
                            unsigned int pos,
                            BYTE data)
{
    debugss(ssFloppyDisk, INFO, "%s: maxTrack (%d) tracks_m(%d)\n", __FUNCTION__,
            maxTrack_m, numTracks_m);

    // Currently only 40 and 80 are supported.
    if ((maxTrack_m != 40) && (maxTrack_m != 80))
    {
        debugss(ssFloppyDisk, ERROR, "%s: Invalid maxTrack: %d\n", __FUNCTION__, maxTrack_m);
        return false;
    }

    if (maxTrack_m != numTracks_m)
    {
        if ((maxTrack_m == 80) && (numTracks_m = 40))
        {
            debugss(ssFloppyDisk, INFO, "%s: max - 80 trk_m - 40\n", __FUNCTION__);
            track /= 2;
        }

        else if ((maxTrack_m == 40) && (numTracks_m == 80))
        {
            debugss(ssFloppyDisk, INFO, "%s: max - 40 trk_m - 80\n", __FUNCTION__);
            track *= 2;
        }
    }

    if (initialized_m)
    {
        if ((track < tracksPerSide_c) && (pos < bytesPerTrack_c))
        {
            debugss(ssFloppyDisk, ALL, "%s: track(%d) pos(%d) = %d\n", __FUNCTION__, track,
                    pos, rawImage_m[side][track][pos]);
            rawImage_m[side][track][pos] = data;
            return true;
        }

        else
        {
            debugss(ssFloppyDisk, ERROR, "%s: Out of Range - track(%d) pos(%d)\n",
                    __FUNCTION__, track, pos);
            return false;
        }
    }

    else
    {
        debugss(ssFloppyDisk, ERROR, "%s: disk not initialized\n", __FUNCTION__);
        return false;
    }
}

void
SoftSectoredDisk::getControlInfo(unsigned int pos,
                                 bool& hole,
                                 bool& writeProtect)
{
    if (initialized_m)
    {
        hole         = defaultHoleStatus(pos);
        writeProtect = checkWriteProtect();
    }

    else
    {
        hole         = true;
        writeProtect = false;
    }

    debugss(ssFloppyDisk, INFO, "%s: init: %d hole: %d wp: %d\n", __FUNCTION__,
            initialized_m, hole, writeProtect);
}


bool
SoftSectoredDisk::defaultHoleStatus(unsigned int pos)
{
    debugss(ssFloppyDisk, ALL, "%s: pos = %d ", __FUNCTION__, pos);

    // check for index hole
    if (pos < 128) /// \todo ??? does this need to change based on the density
    {
        debugss_nts(ssFloppyDisk, ALL, "index hole\n");
        return true;
    }

    debugss_nts(ssFloppyDisk, ALL, "no hole\n");
    return false;

}

bool
SoftSectoredDisk::readTD0(const char* name)
{
    debugss(ssFloppyDisk, ERROR, "%s: Not supported\n", __FUNCTION__);
    return false;
}

bool
SoftSectoredDisk::readIMD(const char* name)
{
    std::ifstream     file;
    unsigned long int fileSize;
    unsigned long int pos = 0;

    BYTE*             buf;

    debugss(ssFloppyDisk, INFO, "%s: file: %s\n", __FUNCTION__, name);

    file.open(name, std::ios::binary);
    file.seekg(0, std::ios::end);
    fileSize = file.tellg();
    file.seekg(0, std::ios::beg);

    buf      = new BYTE[fileSize];
    file.read((char*) buf, fileSize);
    file.close();

    while (buf[pos] != 0x1a)
    {
        if (pos == fileSize)
        {
            delete [] buf;

            return (false);
        }
    }

    // get past the EOF character.
    pos++;

    BYTE            sectorOrder[256];
    BYTE            sectorCylMapping[256];
    BYTE            sectorHeadMapping[256];
    unsigned char   mode, cyl, head, numSec, sectorSizeKey;
    int             sectorSize;
    bool            sectorCylMap, sectorHeadMap;
    Track::Density  density;
    Track::DataRate dataRate;

    do
    {
        // mode
        mode = buf[pos++];

        switch (mode)
        {
            case 0:
                density  = Track::singleDensity;
                dataRate = Track::dr_500kbps;
                debugss(ssFloppyDisk, INFO, "%s: FM/500 kbps\n", __FUNCTION__);
                break;

            case 1:
                density  = Track::singleDensity;
                dataRate = Track::dr_300kbps;
                debugss(ssFloppyDisk, INFO, "%s: FM/300 kbps\n", __FUNCTION__);
                break;

            case 2:
                density  = Track::singleDensity;
                dataRate = Track::dr_250kbps;
                debugss(ssFloppyDisk, INFO, "%s: FM/250 kbps\n", __FUNCTION__);
                break;

            case 3:
                density  = Track::doubleDensity;
                dataRate = Track::dr_500kbps;
                debugss(ssFloppyDisk, INFO, "%s: MFM/500 kbps\n", __FUNCTION__);
                break;

            case 4:
                density  = Track::doubleDensity;
                dataRate = Track::dr_300kbps;
                debugss(ssFloppyDisk, INFO, "%s: MFM/300 kbps\n", __FUNCTION__);
                break;

            case 5:
                density  = Track::doubleDensity;
                dataRate = Track::dr_250kbps;
                debugss(ssFloppyDisk, INFO, "%s: MFM/250 kbps\n", __FUNCTION__);
                break;

            default:
                density  = Track::density_Unknown;
                dataRate = Track::dr_Unknown;
                debugss(ssFloppyDisk, WARNING, "%s: Unknown density/data rate: %d\n", __FUNCTION__,
                        mode);
                break;

        }

        cyl           = buf[pos++];
        debugss(ssFloppyDisk, ALL, "%s: Cylinder:    %d\n", __FUNCTION__, cyl);

        head          = buf[pos++];

        // Check flags.
        sectorCylMap  = ((head & 0x80) != 0);
        sectorHeadMap = ((head & 0x40) != 0);

        // mask off any flags
        head         &= 1;
        debugss(ssFloppyDisk, ALL, "%s: Head:    %d\n", __FUNCTION__, head);

        Track* trk = new Track(head, cyl);

        numSec        = buf[pos++];
        debugss(ssFloppyDisk, ALL, "%s: Num Sectors:    %d\n", __FUNCTION__, numSec);

        sectorSizeKey = buf[pos++];

        if (sectorSizeKey < 7)
        {
            sectorSize = 1 << (sectorSizeKey + 7);
        }

        else
        {
            debugss(ssFloppyDisk, ERROR, "%s: Sector Size unknown: %d\n", __FUNCTION__,
                    sectorSizeKey);
            delete [] buf;
            delete trk;

            return (false);
        }

        debugss(ssFloppyDisk, ALL, "%s: Sector Size: %d (%d)\n", __FUNCTION__, sectorSize,
                sectorSizeKey);

        for (int i = 0; i < numSec; i++)
        {
            sectorOrder[i] = buf[pos++];
        }

        if (sectorCylMap)
        {
            debugss(ssFloppyDisk, INFO, "%s: Sector Cylinder Map Present\n", __FUNCTION__);

            for (int i = 0; i < numSec; i++)
            {
                sectorCylMapping[i] = buf[pos++];
            }
        }

        if (sectorHeadMap)
        {
            debugss(ssFloppyDisk, INFO, "%s: Sector Head Map Present\n", __FUNCTION__);

            for (int i = 0; i < numSec; i++)
            {
                sectorHeadMapping[i] = buf[pos++];
            }
        }

        for (int i = 0; i < numSec; i++)
        {
            unsigned char sectorType = buf[pos++];

            debugss(ssFloppyDisk, INFO, "%s: Sector: %d(%d) Type: %d\n", __FUNCTION__,
                    sectorOrder[i], i, sectorType);

            if (sectorType == 0x00)
            {
                // Sector Data Unavailable
                debugss(ssFloppyDisk, INFO, "%s: Sector Size unknown: %d\n", __FUNCTION__,
                        sectorSizeKey);
            }

            else if (sectorType > 0x08)
            {
                // Out of Range.
                debugss(ssFloppyDisk, ERROR, "%s: Out of Range: %d\n", __FUNCTION__,
                        sectorType);
                delete [] buf;

                return false;
            }

            else
            {
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

    debugss(ssFloppyDisk, INFO, "%s: Read successful.\n", __FUNCTION__);

    return true;
}

bool
SoftSectoredDisk::readRaw(const char* name)
{
    // Currently just supporting the RAW HDOS 3.0 disk images... 40 track, single density,
    // single sided, 10 sectors/track, 256 bytes/sector.
    std::ifstream     file;
    unsigned long int fileSize;
    unsigned long int pos = 0;
    BYTE*             buf;

    file.open(name, std::ios::binary);

    if (!file.is_open())
    {
        debugss(ssFloppyDisk, ERROR, "%s: Unable to open file: %s\n", __FUNCTION__, name);
        return false;
    }

    file.seekg(0, std::ios::end);
    fileSize = file.tellg();
    file.seekg(0, std::ios::beg);

    buf      = new BYTE[fileSize];
    file.read((char*) buf, fileSize);

    file.close();

    debugss(ssFloppyDisk, INFO, "%s: RAW File: %s\n", __FUNCTION__, name);

    if (fileSize != (256 * 10 * 40))
    {
        debugss(ssFloppyDisk, ERROR, "%s: Invalid File Size: %s - %ld\n", __FUNCTION__, name,
                fileSize);
        delete [] buf;

        return false;
    }

    for (int trk = 0; trk < 40; trk++)
    {
        Track* track = new Track(0, trk);

        for (int sect = 0; sect < 10; sect++, pos += 256)
        {
            Sector* sector = new Sector(0, trk, sect, 256, &buf[pos]);

            track->addSector(sector);
            track->setDensity(Track::singleDensity);
            track->setDataRate(Track::dr_300kbps);
        }

        tracks_m[0].push_back(track);
    }

    numTracks_m   = 40;
    numHeads_m    = 1;
    initialized_m = false;
    return true;
}

bool
SoftSectoredDisk::readRaw8(const char* name)
{
    // Currently just supporting the RAW 8" Disks .
    int               numTrks_c        = 77;
    int               numSectors_c     = 26;
    int               bytesPerSector_c = 128;

    std::ifstream     file;
    unsigned long int fileSize;
    unsigned long int pos = 0;
    BYTE*             buf;

    file.open(name, std::ios::binary);

    if (!file.is_open())
    {
        debugss(ssFloppyDisk, ERROR, "%s: Unable to open file: %s\n", __FUNCTION__, name);
        return false;
    }

    file.seekg(0, std::ios::end);
    fileSize = file.tellg();
    file.seekg(0, std::ios::beg);

    buf      = new BYTE[fileSize];
    file.read((char*) buf, fileSize);

    file.close();

    debugss(ssFloppyDisk, INFO, "%s: RAW File: %s\n", __FUNCTION__, name);

    switch (fileSize)
    {
        case 256256:

            break;

        case 512512:

            break;

    }

    if (fileSize != (bytesPerSector_c * numSectors_c * numTrks_c))
    {
        debugss(ssFloppyDisk, ERROR, "%s: Invalid File Size: %s - %ld\n", __FUNCTION__, name,
                fileSize);
        delete [] buf;

        return false;
    }

    for (int trk = 0; trk < numTrks_c; trk++)
    {
        Track* track = new Track(0, trk);

        for (int sect = 0; sect < numSectors_c; sect++, pos += bytesPerSector_c)
        {
            Sector* sector = new Sector(0, trk, sect, bytesPerSector_c, &buf[pos]);

            track->addSector(sector);
            track->setDensity(Track::singleDensity);
            track->setDataRate(Track::dr_500kbps);
        }

        tracks_m[0].push_back(track);
    }

    numTracks_m   = numTrks_c;
    numHeads_m    = 1;
    initialized_m = false;
    return true;
}


void
SoftSectoredDisk::determineDiskFormat(const char* name,
                                      DiskImageFormat& format)
{
    debugss(ssFloppyDisk, INFO, "%s: Name: %s\n", __FUNCTION__, name);
    /// \todo implement
    format = dif_Unknown;
}

void
SoftSectoredDisk::dump()
{
    printf("SoftSectored - dump\n");

    for (int head = 0; head < numHeads_m; head++)
    {
        for (int track = 0; track < numTracks_m; track++)
        {
            printf("  Head: %d  Track: %d\n", head, track);

            tracks_m[head][track]->dump();
        }
    }
}

bool
SoftSectoredDisk::readSectorData(BYTE side,
                                 BYTE track,
                                 BYTE sector,
                                 WORD pos,
                                 BYTE& data)
{
    debugss(ssFloppyDisk, INFO, "%s: side: %d track: %d sector: %d pos: %d\n",
            __FUNCTION__, side, track, sector, pos);

    return tracks_m[side][track]->readSectorData(sector, pos, data);
}

void
SoftSectoredDisk::eject(const char* name)
{

}
