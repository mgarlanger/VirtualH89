///
/// \file HardSectoredDisk.cpp
///
///
/// \date   Sep 9, 2012
/// \author Mark Garlanger
///

#include "HardSectoredDisk.h"
#include "logger.h"

/// \cond
#include <cstring>
/// \endcond

HardSectoredDisk::HardSectoredDisk(const char* name)
{
    FILE* file;

    initialized_m = true;
    memset(rawImage_m, 0, maxHeads_c * bytesPerTrack_c * maxTracksPerSide_c);

    if ((file = fopen(name, "r")) != nullptr)
    {
        unsigned long readCount;
        int           trk  = 0;
        int           head = 0;

        do
        {
            if (trk == 80)
            {
                // Check to see if we read both sides.
                if (head == 1)
                {
                    break;
                }

                trk = 0;
                head++;
            }

            if ((readCount =
                     fread(&rawImage_m[head][trk][0], bytesPerTrack_c, 1, file)) == 1)
            {
                trk++;
            }
            else if ((trk == 0) && (head == 1))
            {
                // check to see if we have to adjust side and track.
                head = 0;
                trk  = 80;
            }
        }
        while (readCount == 1);

        tracks_m = trk;
        sides_m  = head + 1;
        debugss(ssFloppyDisk, ALL, "Sides: %d  Tracks: %d\n", sides_m, tracks_m);
        fclose(file);
    }
    else
    {
        debugss(ssFloppyDisk, ERROR, "unable to open file - %s\n", name);
        initialized_m = false;
    }

    if (initialized_m)
    {
        debugss(ssFloppyDisk, INFO, "Success %s\n", name);
    }
}

HardSectoredDisk::HardSectoredDisk()
{
    debugss(ssFloppyDisk, ALL, "blank disk\n");

    // clear disk
    bzero(rawImage_m, maxHeads_c * bytesPerTrack_c * maxTracksPerSide_c);

    tracks_m      = maxTracksPerSide_c;
    sides_m       = maxHeads_c;

    initialized_m = true;
    setWriteProtect(false);
}

HardSectoredDisk::~HardSectoredDisk()
{

}

bool
HardSectoredDisk::readData(BYTE          side,
                           BYTE          track,
                           unsigned long pos,
                           BYTE&         data)
{
    debugss(ssFloppyDisk, ALL, "maxTrack (%d) tracks_m(%d)\n", maxTrack_m,
            tracks_m);

    // Currently only 40 and 80 are supported.
    if ((maxTrack_m != 40) && (maxTrack_m != 80))
    {
        debugss(ssFloppyDisk, ERROR, "Invalid maxTrack: %d\n", maxTrack_m);
        return false;
    }

    if (maxTrack_m != tracks_m)
    {
        if ((maxTrack_m == 80) && (tracks_m == 40))
        {
            debugss(ssFloppyDisk, INFO, "max - 80 trk_m - 40 | track - %d  newTrack - %d\n",
                    track, track / 2);
            track /= 2;
        }
        else if ((maxTrack_m == 40) && (tracks_m == 80))
        {
            debugss(ssFloppyDisk, INFO, "max - 40 trk_m - 80\n");
            track *= 2;
        }
    }

    if (initialized_m)
    {
        if ((track < maxTracksPerSide_c) && (pos < bytesPerTrack_c))
        {
            debugss(ssFloppyDisk, INFO, "side(%d) track(%d) pos(%lu) = %d\n",
                    side, track, pos, rawImage_m[side][track][pos]);
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
HardSectoredDisk::writeData(BYTE          side,
                            BYTE          track,
                            unsigned long pos,
                            BYTE          data)
{
    debugss(ssFloppyDisk, ALL, "maxTrack (%d) tracks_m(%d)\n", maxTrack_m, tracks_m);

    // Currently only 40 and 80 are supported.
    if ((maxTrack_m != 40) && (maxTrack_m != 80))
    {
        debugss(ssFloppyDisk, ERROR, "Invalid maxTrack: %d\n", maxTrack_m);
        return false;
    }

    if (maxTrack_m != tracks_m)
    {
        if ((maxTrack_m == 80) && (tracks_m == 40))
        {
            debugss(ssFloppyDisk, INFO, "max - 80 trk_m - 40\n");
            track /= 2;
        }
        else if ((maxTrack_m == 40) && (tracks_m == 80))
        {
            debugss(ssFloppyDisk, INFO, "max - 40 trk_m - 80\n");
            track *= 2;
        }
    }

    if (initialized_m)
    {
        if ((track < maxTracksPerSide_c) && (pos < bytesPerTrack_c))
        {
            rawImage_m[side][track][pos] = data;
            debugss(ssFloppyDisk, ALL, "side (%d) track(%d) pos(%lu) = %d\n",
                    side,
                    track,
                    pos, rawImage_m[side][track][pos]);
            return true;
        }
        else
        {
            debugss(ssFloppyDisk, ERROR, "Out of Range - track(%d) pos(%lu)\n", track, pos);
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
HardSectoredDisk::getControlInfo(unsigned long pos,
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
        // No disk in drive so hole light is on.
        hole         = true;

        // write protect sensor is not blocked.
        writeProtect = false;
    }
}

bool
HardSectoredDisk::defaultHoleStatus(unsigned long pos)
{
    debugss(ssFloppyDisk, ALL, "pos = %lu ", pos);

    // check for a sector hole
    if ((pos % 320) < 64)
    {
        debugss_nts(ssFloppyDisk, ALL, "sector hole\n");
        return true;
    }

    // check for index hole
    if ((pos > 3040) && (pos < 3104))
    {
        debugss_nts(ssFloppyDisk, ALL, "index hole\n");
        return true;
    }

    debugss_nts(ssFloppyDisk, ALL, "no hole\n");

    return false;
}

void
HardSectoredDisk::dump()
{
    debugss(ssFloppyDisk, ALL, "Not Implemented\n");
}

void
HardSectoredDisk::eject(const char* name)
{
    FILE* file;
    debugss(ssFloppyDisk, ALL, "Save: %s\n", name);

    if ((file = fopen(name, "wb")) != nullptr)
    {
        unsigned long readCount;

        for (int head = 0; head < sides_m; head++)
        {
            for (int track = 0; track < tracks_m; track++)
            {
                if ((readCount =
                         fwrite(&rawImage_m[head][track][0], bytesPerTrack_c, 1, file)) != 1)
                {
                    debugss(ssFloppyDisk, ERROR, "Unable to save file: %s head: %d track: %d\n",
                            name, head, track);
                }
            }
        }

        fclose(file);
    }
    else
    {
        debugss(ssFloppyDisk, WARNING, "unable to save file - %s\n", name);
    }
}

bool
HardSectoredDisk::readSectorData(BYTE  side,
                                 BYTE  track,
                                 BYTE  sector,
                                 WORD  pos,
                                 BYTE& data)
{
    debugss(ssFloppyDisk, ALL, "Not Implemented\n");
    return false;
}
