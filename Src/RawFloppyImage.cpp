/// \file RawFloppyImage.cpp
///
/// \date Feb 2, 2016
/// \author Douglas Miller
///

#include "RawFloppyImage.h"
#include "logger.h"
#include <sys/types.h>
#include <unistd.h>
#include <fcntl.h>
#include <sys/stat.h>
#include <unistd.h>
#include <string.h>

void RawFloppyImage::getAddrMark(BYTE *tp, int nbytes,
                                 int& id_tk, int& id_sd, int& id_sc, int& id_sl)
{
    id_tk = -1;
    id_sd = -1;
    int indexes = 0;

    while (nbytes > 0)
    {
        if (*tp == GenericFloppyFormat::INDEX_AM_BYTE)
        {
            if (indexes > 1)
            {
                // error...
                debugss(ssRawFloppyImage, WARNING, "%s: no sectors found\n", __FUNCTION__);
                break;
            }

            ++indexes;
        }

        else if (*tp == GenericFloppyFormat::ID_AM_BYTE)
        {
            id_tk = tp[1];
            id_sd = tp[2];
            id_sc = tp[3];
            id_sl = tp[4];
            break;
        }

        ++tp;
        --nbytes;
    }
}

void RawFloppyImage::eject(char const *file)
{
    // flush data...
    cacheTrack(-1, -1);
    close(imageFd_m);
    imageFd_m = -1;
}

void RawFloppyImage::dump()
{
}

RawFloppyImage::RawFloppyImage(GenericDiskDrive *drive, std::vector<std::string> argv):
    GenericFloppyDisk(),
    imageName_m(NULL),
    imageFd_m(-1),
    trackBuffer_m(NULL),
    bufferedTrack_m(-1),
    bufferedSide_m(-1),
    bufferOffset_m(0),
    bufferDirty_m(false),
    hypoTrack_m(false),
    hyperTrack_m(false),
    interlaced_m(false),
    gapLen_m(0),
    indexGapLen_m(0),
    writePos_m(-1),
    trackWrite_m(false)
{
    if (argv.size() < 1)
    {
        debugss(ssRawFloppyImage, WARNING, "%s: no file specified\n", __FUNCTION__);
        return;
    }

    const char *name = strdup(argv[0].c_str());

    // Now look for hints on disk format...
    bool sd = false, dd = false;
    bool ss = false, ds = false;
    bool st = false, dt = false;
    int media = 0;

    for (int x = 1; x < argv.size(); ++x)
    {
        if (argv[x].compare("sd") == 0)
        {
            sd = true;
        }

        else if (argv[x].compare("dd") == 0)
        {
            dd = true;
        }

        else if (argv[x].compare("ss") == 0)
        {
            ss = true;
        }

        else if (argv[x].compare("ds") == 0)
        {
            ds = true;
        }

        else if (argv[x].compare("st") == 0)
        {
            st = true;
        }

        else if (argv[x].compare("dt") == 0)
        {
            dt = true;
        }

        else if (argv[x].compare("5") == 0)
        {
            media = 5;
        }

        else if (argv[x].compare("8") == 0)
        {
            media = 8;
        }

        else if (argv[x].compare("rw") == 0)
        {
            writeProtect_m = false;
        }

        else
        {
            debugss(ssRawFloppyImage, WARNING, "%s: unrecognized hint - %s\n", argv[x].c_str());
        }
    }

    if (!writeProtect_m && access(name, W_OK) != 0)
    {
        debugss(ssRawFloppyImage, WARNING, "Image not writeable: %s\n", name);
        writeProtect_m = true;
    }

    int fd = open(name, writeProtect_m ? O_RDONLY : O_RDWR);

    if (fd < 0)
    {
        debugss(ssRawFloppyImage, ERROR, "%s: unable to open file - %s\n", __FUNCTION__, name);
        return;
    }

    // First examine at least one track, but we don't know the format...
    // So get enough data for 1.5 DD tracks on this drive...
    int nbytes = drive->getRawBytesPerTrack() * 2;
    nbytes += nbytes / 2;
    trackBuffer_m = (BYTE *)malloc(nbytes);

    if (trackBuffer_m == NULL)
    {
        debugss(ssRawFloppyImage, ERROR, "%s: unable to allocate track buffer of %d bytes\n", __FUNCTION__, nbytes);
        close(fd);
        return;
    }

    int n = read(fd, trackBuffer_m, nbytes);

    if (n != nbytes)
    {
        debugss(ssRawFloppyImage, ERROR, "%s: unable to read a track\n", __FUNCTION__);
        close(fd);
        return;
    }

    BYTE *tp = trackBuffer_m;
    BYTE *idx = NULL;
    BYTE *dat = NULL;
    BYTE *end = NULL;
    int trklen = 0;
    int seclen = 0;
    int numsec = 0;
    int idxgap = 0;
    int gaplen = 0;
    int id_tk = -1;
    int id_sd = -1;
    int id_sc = -1;
    int id_sl = -1;

    while (nbytes > 0)
    {
        if (*tp == GenericFloppyFormat::INDEX_AM_BYTE)
        {
            if (idx == NULL)
            {
                idx = tp;
            }

            else
            {
                trklen = (tp - idx);
                // We could continue into next track, and check how side-1
                // is handled.
                break;
            }
        }

        else if (*tp == GenericFloppyFormat::ID_AM_BYTE)
        {
            if (id_tk == -1)
            {
                id_tk = tp[1];
            }

            else if (id_tk != tp[1])
            {
                debugss(ssRawFloppyImage, WARNING, "%s: inconsistent track number %d (%d)\n", __FUNCTION__, tp[1], id_tk);
            }

            if (id_sd == -1)
            {
                id_sd = tp[2];
            }

            else if (id_sd != tp[2])
            {
                debugss(ssRawFloppyImage, WARNING, "%s: inconsistent side number %d (%d)\n", __FUNCTION__, tp[2], id_sd);
            }

            int sl = tp[4] & 0x03;
            sl = (128 << sl);

            if (seclen != 0 && sl != seclen)
            {
                debugss(ssRawFloppyImage, WARNING, "%s: inconsistent sector length %d (%d)\n", __FUNCTION__, sl, seclen);
            }

            seclen = sl;
        }

        else if (*tp == GenericFloppyFormat::DATA_AM_BYTE)
        {
            if (dat == NULL)
            {
                dat = tp + 1;
                idxgap = dat - trackBuffer_m;
            }

            else if (gaplen == 0)
            {
                gaplen = tp + 1 - end;
            }

            else if (gaplen != tp + 1 - end)
            {
                debugss(ssRawFloppyImage, WARNING, "%s: inconsistent data gap %d (%d)\n", __FUNCTION__, tp + 1 - end, gaplen);
            }

            ++numsec;
            tp += seclen;
            nbytes -= seclen;
            end = tp + 1;
        }

        ++tp;
        --nbytes;
    }

    if (trklen == 0 || seclen == 0 || gaplen == 0 || numsec == 0 ||
            id_tk != 0 || id_sd != 0)
    {
        debugss(ssRawFloppyImage, ERROR, "%s: format not recognized\n", __FUNCTION__);
        close(fd);
        return;
    }

    // Now check next track (we have partial, at least) at first ID_AM and see
    // about any clues to track/side arrangement.
    if (nbytes > 0 && *tp == GenericFloppyFormat::INDEX_AM_BYTE)
    {
        ++tp;
        --nbytes;
    }

    getAddrMark(tp, nbytes, id_tk, id_sd, id_sc, id_sl);

    // it's really an error if not found... must be more than one track.
    if (id_tk >= 0 && id_sd >= 0)
    {
        if (id_tk == 1 && id_sd == 0)
        {
            // either SS or image has side 2 in last half...
            interlaced_m = false; // already done.
        }

        else if (id_tk == 0 && id_sd == 1)
        {
            // interlaced image.
            debugss(ssRawFloppyImage, INFO, "%s: setting interlaced \n", __FUNCTION__);
            interlaced_m = true;
            numSides_m = 2;
        }

        else
        {
            debugss(ssRawFloppyImage, ERROR, "%s: invalid track/side layout\n", __FUNCTION__);
        }
    }

    trackLen_m = trklen;
    struct stat stb;
    fstat(fd, &stb);
    int est_trks = stb.st_size / trackLen_m;
    debugss(ssRawFloppyImage, INFO, "%s: estimated number of tracks %d, trklen=%d\n", __FUNCTION__, est_trks, trackLen_m);

    if (est_trks == 77 || est_trks == 154)
    {
        // if interlaced, we know already.
        mediaSize_m = 8;
        numTracks_m = 77;

        if (est_trks > 77)
        {
            numSides_m = 2;
        }

        else
        {
            numSides_m = 1;
        }
    }

    else if (est_trks == 40 || est_trks == 80 || est_trks == 160)
    {
        mediaSize_m = 5;

        if (est_trks > 80)
        {
            numSides_m = 2;
            numTracks_m = 80;
        }

        else if (est_trks < 80)
        {
            numSides_m = 1;
            numTracks_m = 40;
        }
    }

    else
    {
        debugss(ssRawFloppyImage, ERROR, "%s: invalid image size\n", __FUNCTION__);
    }

    imageFd_m = fd; // need this for cacheTrack()...

    if (mediaSize_m == 5 && est_trks == 80 && !interlaced_m)
    {
        // hypoTrack_m, hyperTrack_m still false, numTracks_m == 0, but must have trackLen_m
        if (cacheTrack(0, 40))
        {
            tp = trackBuffer_m;
            nbytes = trackLen_m;
            getAddrMark(tp, nbytes, id_tk, id_sd, id_sc, id_sl);

            if (id_tk >= 0 && id_sd >= 0)
            {
                if (id_tk == 40 && id_sd == 0)
                {
                    // must be SS 80-trk media...
                    numSides_m = 1;
                    numTracks_m = 80;
                }

                else if (id_tk == 0 && id_sd == 1)
                {
                    // non-interlaced double-sided media...
                    numSides_m = 2;
                    numTracks_m = 40;
                }

                else
                {
                    debugss(ssRawFloppyImage, ERROR, "%s: invalid track/side layout\n", __FUNCTION__);
                }
            }

            else
            {
                // probably an error.
                numSides_m = 2;
                numTracks_m = 40;
            }
        }
    }

    numSectors_m = numsec;
    gapLen_m = gaplen;
    indexGapLen_m = idxgap;
    secSize_m = seclen;
    nbytes = drive->getRawBytesPerTrack();

    if (nbytes == trackLen_m)
    {
        doubleDensity_m = false;
    }

    else if (nbytes * 2 == trackLen_m)
    {
        doubleDensity_m = true;
    }

    else if (dd)
    {
        doubleDensity_m = true;
    }

    else if (sd)
    {
        doubleDensity_m = false;
    }

    else
    {
        debugss(ssRawFloppyImage, ERROR, "%s: invalid track length\n", __FUNCTION__);
    }

    if (numTracks_m == 0)
    {
        if (dt && mediaSize_m == 5)
        {
            numTracks_m = 80;
        }

        else if (st && mediaSize_m == 5)
        {
            numTracks_m = 40;
        }

        else if (st && mediaSize_m == 8)
        {
            numTracks_m = 77;
        }

        else
        {
            debugss(ssRawFloppyImage, ERROR, "%s: can't determine number of tracks\n", __FUNCTION__);
        }
    }

    if (numSides_m == 0)
    {
        if (ds)
        {
            numSides_m = 2;
        }

        else if (ss)
        {
            numSides_m = 1;
        }

        else
        {
            debugss(ssRawFloppyImage, ERROR, "%s: can't determine number of sides\n", __FUNCTION__);
        }
    }

    // TODO: keep oversized buffer or free and alloc exact size needed?

    imageName_m = name;
    bufferedTrack_m = -1;
    bufferedSide_m = -1;
    bufferDirty_m = false;
    debugss(ssRawFloppyImage, ERROR, "mounted %d\" floppy %s: sides=%d tracks=%d spt=%d DD=%s R%s\n",
            mediaSize_m, imageName_m, numSides_m, numTracks_m, numSectors_m, doubleDensity_m ? "yes" : "no", writeProtect_m ? "O" : "W");
}

RawFloppyImage::~RawFloppyImage()
{
    if (imageFd_m < 0)
    {
        return;
    }

    debugss(ssRawFloppyImage, INFO, "unmounted %s\n", imageName_m);
    // flush data...
    cacheTrack(-1, -1);
    close(imageFd_m); // check errors?
    imageFd_m = -1;
}

bool RawFloppyImage::cacheTrack(int side, int track)
{
    if (bufferedSide_m == side && bufferedTrack_m == track)
    {
        return true;
    }

    if (imageFd_m < 0)
    {
        return false;
    }

    if (bufferDirty_m && bufferedSide_m != -1 && bufferedTrack_m != -1)
    {
        // Must write to disk...
        lseek(imageFd_m, bufferOffset_m, SEEK_SET);
        int rd = write(imageFd_m, trackBuffer_m, trackLen_m);
        // TODO: what to do if this failed? Also might not know unitl close().
        bufferDirty_m = false;
    }

    if (side < 0 || track < 0)
    {
        // just flush only.
        return true;
    }

    if (hypoTrack_m)
    {
        if ((track & 1) != 0)
        {
            return false;
        }

        track /= 2;
    }

    else if (hyperTrack_m)
    {
        track *= 2;
    }

    if (interlaced_m)
    {
        bufferOffset_m = (track * numSides_m + side) * trackLen_m;
    }

    else
    {
        bufferOffset_m = (side * numTracks_m + track) * trackLen_m;
    }

    lseek(imageFd_m, bufferOffset_m, SEEK_SET);
    int rd = read(imageFd_m, trackBuffer_m, trackLen_m);

    if (rd != trackLen_m)
    {
        bufferedSide_m = -1;
        bufferedTrack_m = -1;
        return false;
    }

    bufferedSide_m = side;
    bufferedTrack_m = track;
    return true;
}

bool RawFloppyImage::readData(BYTE side, BYTE track, unsigned int pos, int& data)
{
    BYTE d;

    if (!cacheTrack(side, track))
    {
        // Just treat like unformatted disk/track.
        return false;
    }

    d = trackBuffer_m[pos];

    int p = pos;
    p -= indexGapLen_m + secSize_m;
    p %= gapLen_m + secSize_m;

    if (pos <= indexGapLen_m || (p >= 0 && p < gapLen_m))
    {
        if (d == GenericFloppyFormat::DATA_AM_BYTE)
        {
            data = GenericFloppyFormat::DATA_AM;
            return true;
        }

        else if (d == GenericFloppyFormat::ID_AM_BYTE)
        {
            data = GenericFloppyFormat::ID_AM;
            return true;
        }

        else if (d == GenericFloppyFormat::INDEX_AM_BYTE)
        {
            data = GenericFloppyFormat::INDEX_AM;
            return true;
        }
    }

    data = d;
    return true;
}

bool RawFloppyImage::startWrite(BYTE side, BYTE track, unsigned int pos)
{
    if (pos < indexGapLen_m / 2)
    {
        debugss(ssRawFloppyImage, WARNING, "TrackWrite not supported by RawFloppyImage\n");
        trackWrite_m = true;
        writePos_m = pos;
        return false; // Not supported
    }

    else
    {
        // 'pos' is position of DATA_AM, so start writing data at next byte.
        writePos_m = pos + 1;
    }

    debugss(ssRawFloppyImage, INFO, "startWrite pos=%d writePos=%d\n", pos, writePos_m);
    return true;
}

bool RawFloppyImage::stopWrite(BYTE side, BYTE track, unsigned int pos)
{
    debugss(ssRawFloppyImage, INFO, "stopWrite pos=%d writePos=%d\n", pos, writePos_m);
    writePos_m = -1;
    return true;
}

bool RawFloppyImage::writeData(BYTE         side,
                               BYTE         track,
                               unsigned int pos,
                               BYTE         data)
{
    if (checkWriteProtect())
    {
        return false;
    }

    if (trackWrite_m)
    {
        return false;
    }

    if (!cacheTrack(side, track))
    {
        // Just treat like unformatted disk/track.
        return false;
    }

    if (pos != writePos_m)
    {
        // fallen behind, don't trash entire track...
        return false;
    }

    ++writePos_m;

    debugss(ssRawFloppyImage, INFO, "writeData pos=%d data=%02x\n", pos, data);
    // TODO: limit or control access to non-sector bytes?
    trackBuffer_m[pos] = data;
    bufferDirty_m = true;
    return true;
}

bool RawFloppyImage::isReady()
{
    return (imageFd_m >= 0);
}

std::string RawFloppyImage::getMediaName()
{
    return imageName_m;
}
