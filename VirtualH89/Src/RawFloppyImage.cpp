/// \file RawFloppyImage.cpp
///
/// \brief virtual Floppy Disk implementation for soft-sectored media.
/// Supports media image files that have a simplified raw-track format.
///
/// \date Feb 2, 2016
/// \author Douglas Miller
///

#include "RawFloppyImage.h"

#include <sys/types.h>
#include <unistd.h>
#include <fcntl.h>
#include <sys/stat.h>
#include <unistd.h>
#include <string.h>
#include <stdlib.h>

#include "logger.h"
#include "GenericFloppyFormat.h"
#include "GenericDiskDrive.h"


void
RawFloppyImage::getAddrMark(BYTE* tp, int nbytes, int& id_tk, int& id_sd,
                            int& id_sc, int& id_sl)
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

void
RawFloppyImage::eject(char const* file)
{
    // flush data...
    cacheTrack(-1, -1);
    close(imageFd_m);
    imageFd_m = -1;
}

void
RawFloppyImage::dump()
{
}

// TODO: If constructor fails, the drive should not mount this disk!
RawFloppyImage::RawFloppyImage(GenericDiskDrive* drive, std::vector<std::string> argv):
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
    trackWrite_m(false),
    dataPos_m(0),
    dataLen_m(0)
{
    if (argv.size() < 1)
    {
        debugss(ssRawFloppyImage, WARNING, "%s: no file specified\n", __FUNCTION__);
        return;
    }

    char* name  = strdup(argv[0].c_str());

    // Now look for hints on disk format...
    bool  sd    = false, dd = false;
    bool  ss    = false, ds = false;
    bool  st    = false, dt = false;
    int   media = 0;

    for (unsigned int x = 1; x < argv.size(); ++x)
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
            debugss(ssRawFloppyImage, WARNING, "%s: unrecognized hint - %s\n", __FUNCTION__,
                    argv[x].c_str());
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
        free(name);
        return;
    }
    // First examine at least one track, but we don't know the format...
    // So get enough data for 1.5 DD tracks on this drive...
    long nbytes = drive->getRawBytesPerTrack() * 2;
    nbytes       += nbytes / 2;
    trackBuffer_m = new BYTE[nbytes];
    if (trackBuffer_m == NULL)
    {
        debugss(ssRawFloppyImage, ERROR, "%s: unable to allocate track buffer of %ld bytes\n",
                __FUNCTION__, nbytes);
        close(fd);
        free(name);
        return;
    }
    long n = read(fd, trackBuffer_m, nbytes);
    if (n != nbytes)
    {
        debugss(ssRawFloppyImage, ERROR, "%s: unable to read a track\n", __FUNCTION__);
        close(fd);
        free(name);
        return;
    }
    BYTE* tp     = trackBuffer_m;
    BYTE* idx    = NULL;
    BYTE* dat    = NULL;
    BYTE* end    = NULL;
    long  trklen = 0;
    int   seclen = 0;
    int   numsec = 0;
    long  idxgap = 0;
    long  gaplen = 0;
    int   id_tk  = -1;
    int   id_sd  = -1;
    int   id_sc  = -1;
    int   id_sl  = -1;
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
                debugss(ssRawFloppyImage, WARNING, "%s: inconsistent track number %d (%d)\n",
                        __FUNCTION__, tp[1], id_tk);
            }
            if (id_sd == -1)
            {
                id_sd = tp[2];
            }
            else if (id_sd != tp[2])
            {
                debugss(ssRawFloppyImage, WARNING, "%s: inconsistent side number %d (%d)\n",
                        __FUNCTION__, tp[2], id_sd);
            }
            int sl = tp[4] & 0x03;
            sl = (128 << sl);
            if (seclen != 0 && sl != seclen)
            {
                debugss(ssRawFloppyImage, WARNING, "%s: inconsistent sector length %d (%d)\n",
                        __FUNCTION__, sl, seclen);
            }
            seclen = sl;
        }
        else if (*tp == GenericFloppyFormat::DATA_AM_BYTE)
        {
            if (dat == NULL)
            {
                dat    = tp + 1;
                idxgap = dat - trackBuffer_m;
            }
            else if (gaplen == 0)
            {
                gaplen = tp + 1 - end;
            }
            else if (gaplen != tp + 1 - end)
            {
                debugss(ssRawFloppyImage, WARNING, "%s: inconsistent data gap %ld (%ld)\n",
                        __FUNCTION__, tp + 1 - end, gaplen);
            }
            ++numsec;
            tp     += seclen;
            nbytes -= seclen;
            end     = tp + 1;
        }
        ++tp;
        --nbytes;
    }
    if (trklen == 0 || seclen == 0 || gaplen == 0 || numsec == 0 ||
        id_tk != 0 || id_sd != 0)
    {
        debugss(ssRawFloppyImage, ERROR, "%s: format not recognized\n", __FUNCTION__);
        free((void*) name);
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
    getAddrMark(tp, (int) nbytes, id_tk, id_sd, id_sc, id_sl);
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
            numSides_m   = 2;
        }
        else
        {
            debugss(ssRawFloppyImage, ERROR, "%s: invalid track/side layout\n", __FUNCTION__);
        }
    }
    trackLen_m = trklen;
    struct stat stb;
    fstat(fd, &stb);
    long long   est_trks = stb.st_size / trackLen_m;
    debugss(ssRawFloppyImage, INFO, "%s: estimated number of tracks %lld, trklen=%ld\n",
            __FUNCTION__, est_trks, trackLen_m);

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
            numSides_m  = 2;
            numTracks_m = 80;
        }
        else if (est_trks < 80)
        {
            numSides_m  = 1;
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
            tp     = trackBuffer_m;
            nbytes = trackLen_m;
            getAddrMark(tp, (int) nbytes, id_tk, id_sd, id_sc, id_sl);
            if (id_tk >= 0 && id_sd >= 0)
            {
                if (id_tk == 40 && id_sd == 0)
                {
                    // must be SS 80-trk media...
                    numSides_m  = 1;
                    numTracks_m = 80;
                }
                else if (id_tk == 0 && id_sd == 1)
                {
                    // non-interlaced double-sided media...
                    numSides_m  = 2;
                    numTracks_m = 40;
                }
                else
                {
                    debugss(ssRawFloppyImage, ERROR, "%s: invalid track/side layout\n",
                            __FUNCTION__);
                }
            }
            else
            {
                // probably an error.
                numSides_m  = 2;
                numTracks_m = 40;
            }
        }
    }
    numSectors_m  = numsec;
    gapLen_m      = gaplen;
    indexGapLen_m = idxgap;
    secSize_m     = seclen;
    nbytes        = drive->getRawBytesPerTrack();

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
            debugss(ssRawFloppyImage, ERROR, "%s: can't determine number of tracks\n",
                    __FUNCTION__);
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
    if (media != 0 && media != mediaSize_m)
    {
        debugss(ssRawFloppyImage, WARNING, "Media size determined to be %d but hint said %d\n",
                mediaSize_m, media);
    }
    // TODO: keep oversized buffer or free and alloc exact size needed?
    imageName_m     = name;
    bufferedTrack_m = -1;
    bufferedSide_m  = -1;
    bufferDirty_m   = false;
    debugss(ssRawFloppyImage, ERROR,
            "mounted %d\" floppy %s: sides=%d tracks=%d spt=%d DD=%s R%s\n",
            mediaSize_m, imageName_m, numSides_m, numTracks_m, numSectors_m,
            doubleDensity_m ? "yes" : "no", writeProtect_m ? "O" : "W");
}

RawFloppyImage::~RawFloppyImage()
{
    if (imageFd_m < 0)
    {
        return;
    }
    debugss(ssRawFloppyImage, INFO, "unmounted %s\n", imageName_m);
    free(imageName_m);
    // flush data...
    cacheTrack(-1, -1);
    close(imageFd_m); // check errors?
    imageFd_m = -1;
}

bool
RawFloppyImage::cacheTrack(int side,
                           int track)
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
        long rd = write(imageFd_m, trackBuffer_m, trackLen_m);
        if (rd != trackLen_m)
        {
            debugss(ssRawFloppyImage, ERROR, "Cache flush write failed, "
                    "trk %d sid %d, data lost\n",
                    bufferedTrack_m, bufferedSide_m);
        }
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
    long rd = read(imageFd_m, trackBuffer_m, trackLen_m);
    if (rd != trackLen_m)
    {
        bufferedSide_m  = -1;
        bufferedTrack_m = -1;
        return false;
    }
    headPos_m       = 0;
    bufferedSide_m  = side;
    bufferedTrack_m = track;
    return true;
}

bool
RawFloppyImage::findMark(int mark)
{
    int indexCount = 0;
    while (indexCount < 2)
    {
        BYTE d = trackBuffer_m[headPos_m++];

        if (headPos_m >= trackLen_m)
        {
            // in most cases, should never hit this since
            // we started at the address fields (there must be
            // sector data after any address field, before index).
            headPos_m = 0;
            ++indexCount;
            continue;
        }

        if (d == GenericFloppyFormat::ID_AM_BYTE)
        {
            if (mark == GenericFloppyFormat::ID_AM)
            {
                return true;
            }
            headPos_m += 6;
        }
        else if (d == GenericFloppyFormat::DATA_AM_BYTE)
        {
            if (mark == GenericFloppyFormat::DATA_AM)
            {
                return true;
            }
            headPos_m += secSize_m + 2;
            if (mark == GenericFloppyFormat::CRC)
            {
                return true;
            }
            // should never get here, should always start before addr,
            // and find what we're looknig for before this.
            break;
        }
    }
    return false;
}

bool
RawFloppyImage::locateSector(BYTE track, BYTE side, BYTE sector)
{
    if (findMark(GenericFloppyFormat::ID_AM))
    {
        if (track != trackBuffer_m[headPos_m] ||
            side != trackBuffer_m[headPos_m + 1] ||
            sector != trackBuffer_m[headPos_m + 2])
        {
            headPos_m += 6;
            findMark(GenericFloppyFormat::CRC);
            return false;
        }
        headPos_m += 6;
        if (findMark(GenericFloppyFormat::DATA_AM))
        {
            return true;
        }
    }
    return false;
}

bool
RawFloppyImage::readData(BYTE track, BYTE side, BYTE sector, int inSector, int& data)
{
    if (!cacheTrack(side, track))
    {
        // Just treat like unformatted disk/track.
        return false;
    }
    if (inSector < 0)
    {
        if (sector == 0xfd) // read address
        {
            if (findMark(GenericFloppyFormat::ID_AM))
            {
                dataPos_m = headPos_m;
                dataLen_m = dataPos_m + 6;
                data      = GenericFloppyFormat::ID_AM;
                findMark(GenericFloppyFormat::CRC);
                debugss(ssRawFloppyImage, INFO, "read address %d %d\n", dataPos_m, dataLen_m);
                return true;
            }
            data = GenericFloppyFormat::ERROR;
            return true;
        }
        else if (sector == 0xff) // read track
        {
            dataPos_m = 0;
            dataLen_m = trackLen_m;
            data      = GenericFloppyFormat::INDEX_AM;
            debugss(ssRawFloppyImage, INFO, "read track %d %d\n", dataPos_m, dataLen_m);
            return true;
        }
        else if (locateSector(track, side, sector))
        {
            dataPos_m  = headPos_m;
            headPos_m += secSize_m + 2;
            dataLen_m  = dataPos_m + secSize_m;
            data       = GenericFloppyFormat::DATA_AM;
            return true;
        }
        data = GenericFloppyFormat::NO_DATA;
        return true;
    }
    if (dataPos_m < dataLen_m)
    {
        data = trackBuffer_m[dataPos_m++];
    }
    else
    {
        debugss(ssRawFloppyImage, INFO, "data done %d %d %d\n", track, side, sector);
        data = GenericFloppyFormat::CRC;
    }
    return true;
}

bool
RawFloppyImage::writeData(BYTE track, BYTE side, BYTE sector, int inSector,
                          BYTE data, bool dataReady, int& result)
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
    if (inSector < 0)
    {
        if (sector == 0xff || sector == 0xfe) // write track
        {
#if 0
            // density is in 'sector' LSB... use to handle proper track size, etc.

            // This does actually work, in the sense that the pattern is
            // written to the file. But, we have to re-mount the image
            // in order to use it, and we have to deal with changes in density
            // (not to mention other aspects of disk geometry). This all gets
            // trickier if we need to handle formatting a single track and
            // reading back sectors to verify. Really need to  significantly
            // rework this paradigm in order to make it work just like a
            // real disk/controller. Right now, it does not handle changing
            // format on-the-fly let alone track-by-track.
            dataPos_m = 0;
            dataLen_m = trackLen_m;
            result    = GenericFloppyFormat::INDEX_AM;
            debugss(ssRawFloppyImage, INFO, "write track %d %d\n", dataPos_m, dataLen_m);
            return true;
#else
            result = GenericFloppyFormat::ERROR;
            return true;
#endif
        }
        else if (locateSector(track, side, sector))
        {
            dataPos_m  = headPos_m;
            headPos_m += secSize_m + 2;
            dataLen_m  = dataPos_m + secSize_m;
            result     = GenericFloppyFormat::DATA_AM;
            return true;
        }
        result = GenericFloppyFormat::NO_DATA;
        return true;
    }
    if (dataPos_m < dataLen_m)
    {
        if (!dataReady)
        {
            result = GenericFloppyFormat::NO_DATA;
        }
        else
        {
            trackBuffer_m[dataPos_m++] = data;
            bufferDirty_m              = true;
            result                     = data;
        }
    }
    else
    {
        result = GenericFloppyFormat::CRC;
    }
    debugss(ssRawFloppyImage, INFO, "writeData pos=%d data=%02x\n", inSector, data);
    return true;
}

bool
RawFloppyImage::isReady()
{
    return imageFd_m >= 0;
}

std::string
RawFloppyImage::getMediaName()
{
    if (imageName_m == NULL)
    {
        return "BAD";
    }
    return imageName_m;
}
