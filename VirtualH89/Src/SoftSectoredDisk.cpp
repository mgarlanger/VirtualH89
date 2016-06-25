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

/// \cond
#include <fstream>
#include <strings.h>
#include <memory>
/// \endcond

using namespace std;

SoftSectoredDisk::SoftSectoredDisk(const char*     name,
                                   DiskImageFormat format): initialized_m(false)
{
    debugss(ssFloppyDisk, INFO, "Insert Disk: %s\n", name);

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
            debugss(ssFloppyDisk, ERROR, "Unknown disk format: %d\n", format);

            break;
    }
}

SoftSectoredDisk::SoftSectoredDisk()
{
    debugss(ssFloppyDisk, INFO, "Insert Empty Disk\n");

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
SoftSectoredDisk::readData(BYTE          side,
                           BYTE          track,
                           unsigned long pos,
                           BYTE&         data)
{
    debugss(ssFloppyDisk, ALL, "maxTrack (%d) tracks_m(%d)\n", maxTrack_m,
            numTracks_m);

    // Currently only 40 and 80 are supported.
    if ((maxTrack_m != 40) && (maxTrack_m != 80))
    {
        debugss(ssFloppyDisk, ERROR, "Invalid maxTrack: %d\n", maxTrack_m);
        return false;
    }

    if (maxTrack_m != numTracks_m)
    {
        if ((maxTrack_m == 80) && (numTracks_m = 40))
        {
            debugss(ssFloppyDisk, INFO, "max - 80 trk_m - 40\n");
            track /= 2;
        }
        else if ((maxTrack_m == 40) && (numTracks_m == 80))
        {
            debugss(ssFloppyDisk, INFO, "max - 40 trk_m - 80\n");
            track *= 2;
        }
    }

    if (initialized_m)
    {
        if ((track < tracksPerSide_c) && (pos < bytesPerTrack_c))
        {
            debugss(ssFloppyDisk, ALL, "track(%d) pos(%lu) = %d\n", track, pos,
                    rawImage_m[side][track][pos]);
            data = rawImage_m[side][track][pos];
            return true;
        }
        else
        {
            debugss(ssFloppyDisk, ERROR, "range error: track(%d) pos(%lu)\n", track, pos);
            return false;
        }
    }
    else
    {
        debugss(ssFloppyDisk, ERROR, "disk not initialized\n");
        return false;
    }
}

bool
SoftSectoredDisk::writeData(BYTE          side,
                            BYTE          track,
                            unsigned long pos,
                            BYTE          data)
{
    debugss(ssFloppyDisk, INFO, "maxTrack (%d) tracks_m(%d)\n", maxTrack_m, numTracks_m);

    // Currently only 40 and 80 are supported.
    if ((maxTrack_m != 40) && (maxTrack_m != 80))
    {
        debugss(ssFloppyDisk, ERROR, "Invalid maxTrack: %d\n", maxTrack_m);
        return false;
    }

    if (maxTrack_m != numTracks_m)
    {
        if ((maxTrack_m == 80) && (numTracks_m = 40))
        {
            debugss(ssFloppyDisk, INFO, "max - 80 trk_m - 40\n");
            track /= 2;
        }
        else if ((maxTrack_m == 40) && (numTracks_m == 80))
        {
            debugss(ssFloppyDisk, INFO, "max - 40 trk_m - 80\n");
            track *= 2;
        }
    }

    if (initialized_m)
    {
        if ((track < tracksPerSide_c) && (pos < bytesPerTrack_c))
        {
            debugss(ssFloppyDisk, ALL, "track(%d) pos(%lu) = %d\n", track,
                    pos, rawImage_m[side][track][pos]);
            rawImage_m[side][track][pos] = data;
            return true;
        }
        else
        {
            debugss(ssFloppyDisk, ERROR, "Out of Range - track(%d) pos(%lu)\n",
                    track, pos);
            return false;
        }
    }
    else
    {
        debugss(ssFloppyDisk, ERROR, "disk not initialized\n");
        return false;
    }
}

void
SoftSectoredDisk::getControlInfo(unsigned long pos,
                                 bool&         hole,
                                 bool&         writeProtect)
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

    debugss(ssFloppyDisk, INFO, "init: %d hole: %d wp: %d\n", initialized_m, hole, writeProtect);
}


bool
SoftSectoredDisk::defaultHoleStatus(unsigned long pos)
{
    debugss(ssFloppyDisk, ALL, "pos = %ld ", pos);

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
    debugss(ssFloppyDisk, ERROR, "Not supported\n");
    return false;
}

bool
SoftSectoredDisk::readIMD(const char* name)
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

        cyl           = buf[pos++];
        debugss(ssFloppyDisk, ALL, "Cylinder:    %d\n", cyl);

        head          = buf[pos++];

        // Check flags.
        sectorCylMap  = ((head & 0x80) != 0);
        sectorHeadMap = ((head & 0x40) != 0);

        // mask off any flags
        head         &= 1;
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
            debugss(ssFloppyDisk, ERROR, "Sector Size unknown: %d\n",
                    sectorSizeKey);
            delete [] buf;
            delete trk;

            return (false);
        }

        debugss(ssFloppyDisk, ALL, "Sector Size: %d (%d)\n", sectorSize,
                sectorSizeKey);

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
                shared_ptr<Sector> sect = make_shared<Sector>(head,
                                                              cyl,
                                                              sectorOrder[i],
                                                              sectorSize,
                                                              &sectorData[0]);

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
        debugss(ssFloppyDisk, ERROR, "Unable to open file: %s\n", name);
        return false;
    }

    file.seekg(0, std::ios::end);
    fileSize = file.tellg();
    file.seekg(0, std::ios::beg);

    buf      = new BYTE[fileSize];
    file.read((char*) buf, fileSize);

    file.close();

    debugss(ssFloppyDisk, INFO, "RAW File: %s\n", name);

    if (fileSize != (256 * 10 * 40))
    {
        debugss(ssFloppyDisk, ERROR, "Invalid File Size: %s - %ld\n", name,
                fileSize);
        delete [] buf;

        return false;
    }

    for (int trk = 0; trk < 40; trk++)
    {
        Track* track = new Track(0, trk);

        for (int sect = 0; sect < 10; sect++, pos += 256)
        {
            shared_ptr<Sector> sector = make_shared<Sector>(0, trk, sect, 256, &buf[pos]);

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
        debugss(ssFloppyDisk, ERROR, "Unable to open file: %s\n", name);
        return false;
    }

    file.seekg(0, std::ios::end);
    fileSize = file.tellg();
    file.seekg(0, std::ios::beg);

    buf      = new BYTE[fileSize];
    file.read((char*) buf, fileSize);

    file.close();

    debugss(ssFloppyDisk, INFO, "RAW File: %s\n", name);

    switch (fileSize)
    {
        case 256256:

            break;

        case 512512:

            break;

    }

    if (fileSize != (bytesPerSector_c * numSectors_c * numTrks_c))
    {
        debugss(ssFloppyDisk, ERROR, "Invalid File Size: %s - %ld\n", name,
                fileSize);
        delete [] buf;

        return false;
    }

    for (int trk = 0; trk < numTrks_c; trk++)
    {
        Track* track = new Track(0, trk);

        for (int sect = 0; sect < numSectors_c; sect++, pos += bytesPerSector_c)
        {
            shared_ptr<Sector> sector = make_shared<Sector>(0,
                                                            trk,
                                                            sect,
                                                            bytesPerSector_c,
                                                            &buf[pos]);

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
SoftSectoredDisk::determineDiskFormat(const char*      name,
                                      DiskImageFormat& format)
{
    debugss(ssFloppyDisk, INFO, "Name: %s\n", name);
    /// \todo implement
    format = dif_Unknown;
}

void
SoftSectoredDisk::dump()
{
    debugss(ssFloppyDisk, INFO, "SoftSectored - dump\n");

    for (int head = 0; head < numHeads_m; head++)
    {
        for (int track = 0; track < numTracks_m; track++)
        {
            debugss(ssFloppyDisk, INFO, "  Head: %d  Track: %d\n", head, track);

            tracks_m[head][track]->dump();
        }
    }
}

bool
SoftSectoredDisk::readSectorData(BYTE  side,
                                 BYTE  track,
                                 BYTE  sector,
                                 WORD  pos,
                                 BYTE& data)
{
    debugss(ssFloppyDisk, ALL, "side: %d track: %d sector: %d pos: %d\n", side, track, sector, pos);

    // return tracks_m[side][track]->readSectorData(sector, pos, data);
    return false;
}

void
SoftSectoredDisk::eject(const char* name)
{

}
