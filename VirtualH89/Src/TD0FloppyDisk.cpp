///
///  \file TD0FloppyDisk.cpp
///
///  \author Mark Garlanger
///  \date   May 2, 2016
///

#include "TD0FloppyDisk.h"

#include "Track.h"
#include "Sector.h"

#include "GenericFloppyFormat.h"
#include "logger.h"

#include <fstream>

using namespace std;

unsigned char TD0FloppyDisk::d_code[256] = {
    0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
    0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
    0x01, 0x01, 0x01, 0x01, 0x01, 0x01, 0x01, 0x01, 0x01, 0x01, 0x01, 0x01, 0x01, 0x01, 0x01, 0x01,
    0x02, 0x02, 0x02, 0x02, 0x02, 0x02, 0x02, 0x02, 0x02, 0x02, 0x02, 0x02, 0x02, 0x02, 0x02, 0x02,
    0x03, 0x03, 0x03, 0x03, 0x03, 0x03, 0x03, 0x03, 0x03, 0x03, 0x03, 0x03, 0x03, 0x03, 0x03, 0x03,
    0x04, 0x04, 0x04, 0x04, 0x04, 0x04, 0x04, 0x04, 0x05, 0x05, 0x05, 0x05, 0x05, 0x05, 0x05, 0x05,
    0x06, 0x06, 0x06, 0x06, 0x06, 0x06, 0x06, 0x06, 0x07, 0x07, 0x07, 0x07, 0x07, 0x07, 0x07, 0x07,
    0x08, 0x08, 0x08, 0x08, 0x08, 0x08, 0x08, 0x08, 0x09, 0x09, 0x09, 0x09, 0x09, 0x09, 0x09, 0x09,
    0x0A, 0x0A, 0x0A, 0x0A, 0x0A, 0x0A, 0x0A, 0x0A, 0x0B, 0x0B, 0x0B, 0x0B, 0x0B, 0x0B, 0x0B, 0x0B,
    0x0C, 0x0C, 0x0C, 0x0C, 0x0D, 0x0D, 0x0D, 0x0D, 0x0E, 0x0E, 0x0E, 0x0E, 0x0F, 0x0F, 0x0F, 0x0F,
    0x10, 0x10, 0x10, 0x10, 0x11, 0x11, 0x11, 0x11, 0x12, 0x12, 0x12, 0x12, 0x13, 0x13, 0x13, 0x13,
    0x14, 0x14, 0x14, 0x14, 0x15, 0x15, 0x15, 0x15, 0x16, 0x16, 0x16, 0x16, 0x17, 0x17, 0x17, 0x17,
    0x18, 0x18, 0x19, 0x19, 0x1A, 0x1A, 0x1B, 0x1B, 0x1C, 0x1C, 0x1D, 0x1D, 0x1E, 0x1E, 0x1F, 0x1F,
    0x20, 0x20, 0x21, 0x21, 0x22, 0x22, 0x23, 0x23, 0x24, 0x24, 0x25, 0x25, 0x26, 0x26, 0x27, 0x27,
    0x28, 0x28, 0x29, 0x29, 0x2A, 0x2A, 0x2B, 0x2B, 0x2C, 0x2C, 0x2D, 0x2D, 0x2E, 0x2E, 0x2F, 0x2F,
    0x30, 0x31, 0x32, 0x33, 0x34, 0x35, 0x36, 0x37, 0x38, 0x39, 0x3A, 0x3B, 0x3C, 0x3D, 0x3E, 0x3F
};

unsigned char TD0FloppyDisk::d_len[] = {
    2, 2, 3, 3, 3, 4, 4, 4, 4, 5, 5, 5, 6, 6, 6, 7
};


//
// Initialize the decompressor trees and state variables
//
void
TD0FloppyDisk::init_decompress()
{
    unsigned i, j;

    for (i = j = 0; i < N_CHAR; ++i)
    {
        // Walk up
        freq[i]           = 1;
        son[i]            = i + TSIZE;
        parent[i + TSIZE] = i;
    }

    while (i <= ROOT)
    {
        // Back down
        freq[i]   = freq[j] + freq[j + 1];
        son[i]    = j;
        parent[j] = parent[j + 1] = i++;
        j        += 2;
    }

    memset(ring_buff, ' ', sizeof ring_buff);
    freq[TSIZE]  = 0xFFFF;
    parent[ROOT] = Bitbuff = Bits = 0;
    GBr          = SBSIZE - LASIZE;
}

//
// Increment frequency tree entry for a given code
//
void
TD0FloppyDisk::update(int c)
{
    unsigned i, j, k, f, l;

    if (freq[ROOT] == MAX_FREQ)
    {
        // Tree is full - rebuild
        // Halve cumulative freq for leaf nodes
        for (i = j = 0; i < TSIZE; ++i)
        {
            if (son[i] >= TSIZE)
            {
                freq[j] = (freq[i] + 1) / 2;
                son[j]  = son[i];
                ++j;
            }
        }

        // make a tree - first connect children nodes
        for (i = 0, j = N_CHAR; j < TSIZE; i += 2, ++j)
        {
            k = i + 1;
            f = freq[j] = freq[i] + freq[k];
            for (k = j - 1; f < freq[k]; --k)
            {
                ;
            }
            ++k;
            l       = (j - k) * sizeof(freq[0]);

            memmove(&freq[k + 1], &freq[k], l);
            freq[k] = f;
            memmove(&son[k + 1], &son[k], l);
            son[k]  = i;
        }

        // Connect parent nodes
        for (i = 0; i < TSIZE; ++i)
        {
            if ((k = son[i]) >= TSIZE)
            {
                parent[k] = i;
            }
            else
            {
                parent[k] = parent[k + 1] = i;
            }
        }
    }

    c = parent[c + TSIZE];

    do
    {
        k = ++freq[c];
        // Swap nodes if necessary to maintain frequency ordering
        if (k > freq[l = c + 1])
        {
            while (k > freq[++l])
            {
                ;
            }
            freq[c]            = freq[--l];
            freq[l]            = k;
            parent[i = son[c]] = l;
            if (i < TSIZE)
            {
                parent[i + 1] = l;
            }
            parent[j = son[l]] = c;
            son[l]             = i;
            if (j < TSIZE)
            {
                parent[j + 1] = c;
            }
            son[c] = j;
            c      = l;
        }
    }
    while ((c = parent[c]) != 0); // Repeat up to root
}

//
// Get a byte from the input file and flag Eof at end
//
unsigned
TD0FloppyDisk::GetChar()
{
    unsigned c;
    if (pos < size)
    {
        c = buf[pos++];
    }
    else
    {
        c   = 0;
        Eof = true;
    }

    return c;
}

//
// Get a single bit from the input stream
//
unsigned
TD0FloppyDisk::GetBit()
{
    unsigned t;
    if (!Bits--)
    {
        Bitbuff |= GetChar() << 8;
        Bits     = 7;
    }

    t         = (Bitbuff >> 15) & 0x01;
    Bitbuff <<= 1;
    return t;
}

//
// Get a byte from the input stream - NOT bit-aligned
//
unsigned
TD0FloppyDisk::GetByte()
{
    unsigned t;
    if (Bits < 8)
    {
        Bitbuff |= GetChar() << (8 - Bits);
    }
    else
    {
        Bits -= 8;
    }

    t         = Bitbuff >> 8;
    Bitbuff <<= 8;
    return (t & 0xff);
}

//
// Decode a character value from table
//
unsigned
TD0FloppyDisk::DecodeChar()
{
    unsigned c;

    // search the tree from the root to leaves.
    // choose node #(son[]) if input bit == 0
    // choose node #(son[]+1) if input bit == 1
    c = ROOT;
    while ((c = son[c]) < TSIZE)
    {
        c += GetBit();
    }

    update(c -= TSIZE);
    return c;
}

//
// Decode a compressed string index from the table
//
unsigned
TD0FloppyDisk::DecodePosition()
{
    unsigned i, j, c;

    // Decode upper 6 bits from given table
    i = GetByte();
    c = d_code[i] << 6;

    // input lower 6 bits directly
    j = d_len[i >> 4];
    while (--j)
    {
        i = (i << 1) | GetBit();
    }

    return (i & 0x3F) | c;
}

//
// Get a byte from the input file - decompress if required
//
// This implements a state machine to perform the LZSS decompression
// allowing us to decompress the file "on the fly", without having to
// have it all in memory.
//
int
TD0FloppyDisk::getbyte()
{
    unsigned c;

    --GBcheck;
    if (!advanceCompression_m)
    {
        // No compression
        return GetChar();
    }

    for (;; )
    {
        // Decompressor state machine
        if (Eof)
        {
            // End of file has been flagged
            return -1;
        }
        if (!GBstate)
        {
            // Not in the middle of a string
            c = DecodeChar();
            if (c < 256)
            {
                // Direct data extraction
                ring_buff[GBr++] = c;
                GBr             &= (SBSIZE - 1);
                return c;
            }
            GBstate = true; // Begin extracting a compressed string
            GBi     = (GBr - DecodePosition() - 1) & (SBSIZE - 1);
            GBj     = c - 255 + THRESHOLD;
            GBk     = 0;
        }
        if (GBk < GBj)
        {
            // Extract a compressed string
            ring_buff[GBr++] = c = ring_buff[(GBk++ + GBi) & (SBSIZE - 1)];
            GBr             &= (SBSIZE - 1);
            return c;
        }
        GBstate = false; // Reset to non-string state
    }
}

//
// Get a word from the input file via getbyte (for compression)
//
unsigned
TD0FloppyDisk::getword()
{
    unsigned w = getbyte();
    w |= (getbyte() << 8);
    return w;
}



shared_ptr<GenericFloppyDisk>
TD0FloppyDisk::getDiskette(vector<string> argv)
{
    shared_ptr<GenericFloppyDisk> gd = make_shared<TD0FloppyDisk>(argv);

    if (gd->isReady())
    {
        return gd;
    }

    return nullptr;
}


TD0FloppyDisk::TD0FloppyDisk(vector<string> argv): GenericFloppyDisk(),
                                                   imageName_m(nullptr),
                                                   curSector_m(nullptr),
                                                   dataPos_m(0),
                                                   sectorLength_m(0),
                                                   secLenCode_m(0),
                                                   ready_m(true)
{
    if (argv.size() < 1)
    {
        debugss(ssFloppyDisk, WARNING, "no file specified\n");
        return;
    }

    string name(argv[0].c_str());
    debugss(ssFloppyDisk, INFO, "reading: %s\n", name.c_str());

    for (int x = 1; x < argv.size(); ++x)
    {
        if (argv[x].compare("rw") == 0)
        {
            writeProtect_m = false;
        }
    }


    if (!readTD0(name.c_str()))
    {
        ready_m = false;
        debugss(ssFloppyDisk, ERROR, "Read of file %s failed\n", name.c_str());
    }

}

TD0FloppyDisk::~TD0FloppyDisk()
{

}

bool
TD0FloppyDisk::findSector(int side,
                          int track,
                          int sector)
{

    if (hypoTrack_m)
    {
        if ((track & 1) != 0)
        {
            // return false;
        }

        track /= 2;
    }
    else if (hyperTrack_m)
    {
        track *= 2;
    }

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
            debugss(ssFloppyDisk, ERROR, "bad sector size: %d\n", sectorLength_m);
            curSector_m = nullptr;
            return false;
    }

    return true;
}


bool
TD0FloppyDisk::readTD0(const char* name)
{
    ifstream        file;

    Track::Density  density;
    Track::DataRate dataRate;

    debugss(ssFloppyDisk, INFO, "file: %s\n", name);

    file.open(name, ios::binary);
    file.seekg(0, ios::end);
    size = file.tellg();
    file.seekg(0, ios::beg);

    buf  = new BYTE[size];
    file.read((char*) buf, size);
    file.close();

    if ((buf[0] == 'T') && (buf[1] == 'D'))
    {
        debugss(ssFloppyDisk, INFO, "TD0 file with Normal Compression\n");
        advanceCompression_m = false;
    }
    else if ((buf[0] == 't') && (buf[1] == 'd'))
    {
        debugss(ssFloppyDisk, INFO, "TD0 file with Advanced Compression\n");
        advanceCompression_m = true;
        init_decompress();

        delete [] buf;

        return (false);
    }
    else
    {
        debugss(ssFloppyDisk, ERROR, "Not a TD0 file: 0x%02x 0x%02x\n", buf[0], buf[1]);

        delete [] buf;

        return (false);
    }


    debugss(ssFloppyDisk, INFO, "Sequence: %d\n", buf[2]);
    debugss(ssFloppyDisk, INFO, "Check Sequence: %d\n", buf[3]);
    debugss(ssFloppyDisk, INFO, "Teledisk Version %d.%d\n", buf[4] & 0xf0 >> 4,
            buf[4] & 0x0f);
    debugss(ssFloppyDisk, INFO, "Data Rate: %d\n", buf[5]);

    switch (buf[5] & 0x3)
    {
        case 0:
            dataRate = Track::dr_250kbps;
            break;

        case 1:
            dataRate = Track::dr_300kbps;
            break;

        case 2:
            dataRate = Track::dr_500kbps;
            break;

        default:
            debugss(ssFloppyDisk, ERROR, " Unknown(%d)", buf[5]);
            dataRate = Track::dr_Unknown;
            break;
    }
    if (buf[5] & 0x80)
    {
        density         = Track::singleDensity;
        doubleDensity_m = false;
    }
    else
    {
        density         = Track::doubleDensity;
        doubleDensity_m = true;
    }
    debugss(ssFloppyDisk, INFO, "Density: %d\n", density);


    debugss(ssFloppyDisk, INFO, "Drive Type:  %d\n", buf[6]);
    debugss(ssFloppyDisk, INFO, "DOS Allocation flag: %s\n", ((buf[8] != 0) ? "True" : "False"));
    debugss(ssFloppyDisk, INFO, "Sides: %s\n", ((buf[9] == 1) ? "1" : "2"));
    debugss(ssFloppyDisk, INFO, "CRC Check: 0x%02x%02x\n", buf[10], buf[11]);

    pos = 12;

    if (buf[7] & 0x80)
    {
        unsigned char a, b, c;

        debugss(ssFloppyDisk, INFO, "Comment: \n");
        a = getbyte();
        b = getbyte();
        debugss(ssFloppyDisk, INFO, "CRC Check: 0x%02x%02x\n", a, b);

        unsigned commentLen = getword();
        debugss(ssFloppyDisk, INFO, "Data Length: %d\n", commentLen);
        a = getbyte();
        b = getbyte();
        c = getbyte();
        debugss(ssFloppyDisk, INFO, "Year: %d/%d/%d\n", 1900 + a, b, c);
        a = getbyte();
        b = getbyte();
        c = getbyte();
        debugss(ssFloppyDisk, INFO, "Time: %d:%02d:%02d\n", a, b, c);

        // debugss(ssFloppyDisk, INFO, "     Comment: ");

        for (int i = 0; i < commentLen; i++)
        {
            // \todo ignore comment for now, but need to store it, so that when
            // image is written back to disk, it can be written too.
            // a = getbyte();

            getbyte();
        }

    }

    bool done = false;

    // read track
    do
    {
        int sectors  = getbyte();
        int cylinder = getbyte();
        int side     = getbyte();
        density = (side & 0x80) ? Track::singleDensity : Track::doubleDensity;
        side   &= 0x01;

        if (cylinder >= numTracks_m)
        {
            numTracks_m = cylinder + 1;
        }
        int crc = getbyte();
        debugss(ssFloppyDisk, INFO, "Track %d head: %d density: %d CRC: %d\n", cylinder,
                side, density, crc);

        if (sectors == 255)
        {
            done = true;
        }
        else
        {

            shared_ptr<Track> trk = make_shared<Track>(side, cylinder);
            trk->setDensity(density);
            trk->setDataRate(dataRate);

            for (int sectorNum = 0; sectorNum < sectors; sectorNum++)
            {
                int secCyl  = getbyte();
                int secHead = getbyte();
                int secNum  = getbyte();
                int secSize = 0;

                int tmp     = getbyte();
                if (tmp < 7)
                {
                    secSize = 128 << tmp;
                }
                else
                {
                    debugss(ssFloppyDisk, ERROR, "Unknown sector size: %d\n", tmp);
                }

                int secFlags = getbyte();
                int secCRC   = getbyte();

                debugss(ssFloppyDisk, INFO, "Sector %d data:\n", sectorNum);
                debugss(ssFloppyDisk, INFO, "Cyl: %d\n", secCyl);
                debugss(ssFloppyDisk, INFO, "Head: %d\n", secHead);
                debugss(ssFloppyDisk, INFO, "Num: %d\n", secNum);
                debugss(ssFloppyDisk, INFO, "Size: %d\n", secSize);
                debugss(ssFloppyDisk, INFO, "Flags: 0x%02x\n", secFlags);
                debugss(ssFloppyDisk, INFO, "CRC: 0x%02x\n", secCRC);

                // make sure there is data
                if ((secFlags & 0x30) == 0)
                {
                    unsigned      blockSize = getword();

                    // apparently blockSize includes the encoding byte, take it off
                    blockSize--;
                    int           encoding = getbyte();

                    unsigned char block[8096];
                    int           blockPos;
                    switch (encoding)
                    {
                        case 0: // Raw Data

                            for (int count = 0; count < blockSize; count++)
                            {
                                block[count] = getbyte();
                            }
                            break;

                        case 1: // Repeat 2 bytes

                            blockPos = 0;
                            do
                            {
                                int           runlength;
                                unsigned char val[2];

                                runlength = getword();

                                val[0]    = getbyte();
                                val[1]    = getbyte();

                                for (int j = 0; j < runlength; j++)
                                {
                                    block[blockPos++] = val[0];
                                    block[blockPos++] = val[1];
                                }
                            }
                            while (blockPos < secSize);
                            break;

                        case 2: // Run Length Encoding

                            blockPos = 0;
                            do
                            {
                                int code;
                                code = getbyte();

                                if (code == 0)
                                {
                                    // run of raw bytes.
                                    int length;

                                    length = getbyte();
                                    while (length--)
                                    {
                                        block[blockPos++] = getbyte();
                                    }
                                }
                                else
                                {
                                    // run-length encoded.
                                    // length of data that is repeating
                                    int l      = code * 2;
                                    // the number of time to repeat it
                                    int repeat = getbyte();

                                    // instead of copying data to a temp block and copy
                                    // from there, read it into the actual block and
                                    // read the repeated blocks from the previously
                                    // written data.

                                    // read in the data
                                    for (int k = 0; k < l; k++)
                                    {
                                        block[blockPos++] = getbyte();
                                    }

                                    // Now just copy the data from the previous read.
                                    for (int j = 1; j < repeat; j++)
                                    {
                                        for (int k = 0; k < l; k++)
                                        {
                                            // look back at the last read
                                            block[blockPos] = block[blockPos - l];
                                            blockPos++;
                                        }
                                    }
                                }
                            }
                            while (blockPos < secSize);
                            break;

                        default:

                            debugss(ssFloppyDisk, ERROR, "Unknown sector encoding %d\n", encoding);
                            break;

                    }
                    // create sector
                    shared_ptr<Sector> sect = make_shared<Sector>(secHead, secCyl, secNum, secSize,
                                                                  block);

                    // set flags
                    sect->setReadError((secFlags & 0x02) == 0x02);
                    sect->setDeletedDataAddressMark((secFlags & 0x04) == 0x04);

                    // add sector to track
                    trk->addSector(sect);

                }

            }
            // add track to disk
            tracks_m[side].push_back(trk);

        }
    }
    while (!done);

    delete [] buf;

    debugss(ssFloppyDisk, INFO, "Read successful.\n");

    return true;
}


bool
TD0FloppyDisk::readData(BYTE track,
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
        }
        else if (sector == 0xff)
        {
            data = GenericFloppyFormat::INDEX_AM;
        }
        else if (findSector(side, track, sector))
        {
            dataPos_m = 0;
            data      = GenericFloppyFormat::DATA_AM;
        }
        else
        {
            data = GenericFloppyFormat::NO_DATA;
        }

        return true;
    }

    if (sector == 0xfd)
    {
        switch (inSector)
        {
            case 0:
                if (hypoTrack_m)
                {
                    data = track / 2;
                }
                else if (hyperTrack_m)
                {
                    data = track * 2;
                }
                else
                {
                    data = track;
                }
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
TD0FloppyDisk::writeData(BYTE track,
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
        }
        else if (findSector(side, track, sector))
        {
            dataPos_m = 0;
            result    = GenericFloppyFormat::DATA_AM;
        }
        else
        {
            result = GenericFloppyFormat::NO_DATA;
        }
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
TD0FloppyDisk::getMaxSectors(BYTE side,
                             BYTE track)
{
    BYTE sectors = 0;

    if (tracks_m[side][track])
    {
        sectors = tracks_m[side][track]->getMaxSectors();
    }

    return sectors;
}
bool
TD0FloppyDisk::isReady()
{
    return ready_m;
}

void
TD0FloppyDisk::eject(const char* name)
{
    // \todo implement

}

void
TD0FloppyDisk::dump(void)
{
    // \todo implement
}

string
TD0FloppyDisk::getMediaName()
{
    if (!imageName_m)
    {
        return "NONE";
    }

    return imageName_m;
}
