/// \file SectorFloppyImage.cpp
///
/// \brief virtual Floppy Disk implementation for soft-sectored media.
/// Supports media image files that have a sector image of the disk
///
/// \date Feb 2, 2016
/// \author Douglas Miller
///

#include <sys/types.h>
#include <unistd.h>
#include <fcntl.h>
#include <sys/stat.h>
#include <unistd.h>
#include <string.h>

#include "SectorFloppyImage.h"
#include "RawFloppyImage.h"
#include "logger.h"

bool
SectorFloppyImage::checkHeader(BYTE* buf, int n)
{
    BYTE* b = buf;
    int   m = 0;

    while (*b != '\n' && *b != '\0' && b - buf < n)
    {
        BYTE* e;
        int   p = strtoul((char*) b, (char**) &e, 0);

        switch (tolower(*e))
        {
            case 'm':
                if (p != 5 && p != 8)
                {
                    break;
                }

                m          |= 0x01;
                mediaSize_m = p;
                break;

            case 'z':
                if (p != 128 && p != 256 && p != 512 && p != 1024)
                {
                    break;
                }

                m        |= 0x02;
                secSize_m = p;
                break;

            case 'p':
                if (p == 0)
                {
                    break;
                }

                m           |= 0x04;
                numSectors_m = p;
                break;

            case 's':
                if (p < 1 || p > 2)
                {
                    break;
                }

                m         |= 0x08;
                numSides_m = p;
                break;

            case 't':
                if (p == 0)
                {
                    break;
                }

                m          |= 0x10;
                numTracks_m = p;
                break;

            case 'd':
                m              |= 0x20;
                doubleDensity_m = (p != 0);
                break;

            case 'i':
                m           |= 0x40;
                interlaced_m = (p != 0);
                break;

            case 'l': // optional ?
                // m |= 0x80;
                mediaLat_m = p;
                break;

            default:
                return false;
        }

        b = e + 1;
    }

    if (m != 0x7f)
    {
        debugss(ssSectorFloppyImage, INFO, "Failed to interpret header %04x\n", m);
    }

    return (m == 0x7f);
}

void
SectorFloppyImage::eject(char const* file)
{
    // flush data...
    close(imageFd_m);
    imageFd_m = -1;
}

void
SectorFloppyImage::dump()
{
}

// TODO: If constructor fails, the drive should not mount this disk!
SectorFloppyImage::SectorFloppyImage(GenericDiskDrive* drive, std::vector<std::string> argv):
    GenericFloppyDisk(),
    imageName_m(NULL),
    imageFd_m(-1),
    secBuf_m(NULL),
    bufferedTrack_m(-1),
    bufferedSide_m(-1),
    bufferedSector_m(-1),
    bufferOffset_m(0),
    hypoTrack_m(false),
    hyperTrack_m(false),
    interlaced_m(false),
    mediaLat_m(0),
    secLenCode_m(0),
    gapLen_m(0),
    indexGapLen_m(0),
    writePos_m(0),
    trackWrite_m(false),
    dataPos_m(0),
    dataLen_m(0)
{
    if (argv.size() < 1)
    {
        debugss(ssSectorFloppyImage, WARNING, "%s: no file specified\n", __FUNCTION__);
        return;
    }

    const char* name = strdup(argv[0].c_str());

    for (int x = 1; x < argv.size(); ++x)
    {
        if (argv[x].compare("rw") == 0)
        {
            writeProtect_m = false;
        }
    }

    if (!writeProtect_m && access(name, W_OK) != 0)
    {
        debugss(ssSectorFloppyImage, WARNING, "Image not writeable: %s\n", name);
        writeProtect_m = true;
    }

    int fd = open(name, writeProtect_m ? O_RDONLY : O_RDWR);

    if (fd < 0)
    {
        debugss(ssSectorFloppyImage, ERROR, "%s: unable to open file - %s\n", __FUNCTION__, name);
        return;
    }

    BYTE  buf[128];
    off_t end = lseek(fd, (off_t) 0, SEEK_END);

    if (end == 0)
    {
        // empty file... can't access...
        debugss(ssSectorFloppyImage, ERROR, "file is empty - %s\n", name);
        return;
    }

    lseek(fd, end - sizeof(buf), SEEK_SET);
    int  x    = read(fd, buf, sizeof(buf));
    bool done = (x == sizeof(buf) && checkHeader(buf, sizeof(buf)));

    if (!done)
    {
        debugss(ssSectorFloppyImage, ERROR, "file is not SectorFloppyImage (%d,%d) - %s\n", x,
                errno,
                name);
        return;
    }

    hypoTrack_m  = (drive->getNumTracks() > numTracks_m);
    hyperTrack_m = (drive->getNumTracks() < numTracks_m);
    trackLen_m   = (mediaSize_m == 5 ? 3200 : 6400);
    secLenCode_m = ffs(secSize_m); // 128==8, 1024==11

    if (secLenCode_m > 0)
    {
        secLenCode_m -= 8;    // 128==0, 256=1, 512=2, 1024==3
        secLenCode_m &= 0x03; // just to be sure.
    }

    if (doubleDensity_m)
    {
        trackLen_m *= 2;
    }

    secBuf_m    = new BYTE[secSize_m];

    imageFd_m   = fd;
    imageName_m = name;
    debugss(ssSectorFloppyImage, ERROR,
            "mounted %d\" floppy %s: sides=%d tracks=%d spt=%d DD=%s R%s\n",
            mediaSize_m, imageName_m, numSides_m, numTracks_m, numSectors_m,
            doubleDensity_m ? "yes" : "no", writeProtect_m ? "O" : "W");
}

SectorFloppyImage::~SectorFloppyImage()
{
    if (imageFd_m < 0)
    {
        return;
    }

    debugss(ssSectorFloppyImage, INFO, "unmounted %s\n", imageName_m);
    close(imageFd_m); // check errors?
    imageFd_m = -1;
}

GenericFloppyDisk*
SectorFloppyImage::getDiskette(GenericDiskDrive*        drive,
                               std::vector<std::string> argv)
{
    GenericFloppyDisk* gd = new SectorFloppyImage(drive, argv);

    if (!gd->isReady())
    {
        debugss(ssSectorFloppyImage, INFO, "%s: Falling back to RawFloppyImage\n", argv[0].c_str());
        delete gd;
        gd = new RawFloppyImage(drive, argv);
    }

    return gd;
}

bool
SectorFloppyImage::cacheSector(int side, int track, int sector)
{
    if (bufferedSide_m == side && bufferedTrack_m == track && bufferedSector_m == sector)
    {
        return true;
    }

    if (imageFd_m < 0)
    {
        return false;
    }

    if (bufferDirty_m && bufferedSide_m != -1 && bufferedTrack_m != -1 && bufferedSector_m != -1)
    {
        lseek(imageFd_m, bufferOffset_m, SEEK_SET);
        int rd = write(imageFd_m, secBuf_m, secSize_m);
        bufferDirty_m = false;
    }

    if (side < 0 || track < 0 || sector < 0)
    {
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
        bufferOffset_m = ((track * numSides_m + side) * numSectors_m + sector - 1) * secSize_m;
    }

    else
    {
        bufferOffset_m = ((side * numTracks_m + track) * numSectors_m + sector - 1) * secSize_m;
    }

    lseek(imageFd_m, bufferOffset_m, SEEK_SET);
    int rd = read(imageFd_m, secBuf_m, secSize_m);

    if (rd != secSize_m)
    {
        bufferedSide_m   = -1;
        bufferedTrack_m  = -1;
        bufferedSector_m = -1;
        return false;
    }

    bufferedSide_m   = side;
    bufferedTrack_m  = track;
    bufferedSector_m = sector;
    return true;

}

bool
SectorFloppyImage::readData(BYTE track, BYTE side, BYTE sector, int inSector, int& data)
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

        else if (cacheSector(side, track, sector))
        {
            dataPos_m = 0;
            dataLen_m = dataPos_m + secSize_m;
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
        if (inSector < trackLen_m)
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

    if (dataPos_m < dataLen_m)
    {
        data = secBuf_m[dataPos_m++];
    }

    else
    {
        debugss(ssSectorFloppyImage, INFO, "data done %d %d %d\n", track, side, sector);
        data = GenericFloppyFormat::CRC;
    }

    return true;
}

bool
SectorFloppyImage::writeData(BYTE track, BYTE side, BYTE sector, int inSector,
                             BYTE data, bool dataReady, int& result)
{
    if (checkWriteProtect())
    {
        debugss(ssSectorFloppyImage, ERROR, "write protect\n");
        return false;
    }

    if (trackWrite_m)
    {
        debugss(ssSectorFloppyImage, ERROR, "track write\n");
        return false;
    }

    if (inSector < 0)
    {
        if (sector == 0xff || sector == 0xfe)
        {
            result = GenericFloppyFormat::ERROR;
            return true;
        }

        else if (cacheSector(side, track, sector))
        {
            dataPos_m = 0;
            dataLen_m = dataPos_m + secSize_m;
            result    = GenericFloppyFormat::DATA_AM;
            return true;
        }

        result = GenericFloppyFormat::NO_DATA;
        return true;
    }


    debugss(ssSectorFloppyImage, INFO, "writeData pos=%d data=%02x\n", inSector, data);

    if (dataPos_m < dataLen_m)
    {
        if (!dataReady)
        {
            result = GenericFloppyFormat::NO_DATA;
        }

        else
        {
            secBuf_m[dataPos_m++] = data;
            bufferDirty_m         = true;
            result                = data;
        }
    }

    else
    {
        result = GenericFloppyFormat::CRC;
    }

    return true;
}

bool
SectorFloppyImage::isReady()
{
    return (imageFd_m >= 0);
}

std::string
SectorFloppyImage::getMediaName()
{
    if (imageName_m == NULL)
    {
        return "BAD";
    }

    return imageName_m;
}
