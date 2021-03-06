///
/// \name EightInchDisk.cpp
///
///
/// \date Aug 5, 2013
/// \author Mark Garlanger
///
#if 0
#include "EightInchDisk.h"


#include "logger.h"
#include "Track.h"
#include "Sector.h"

/// \cond
#include <iostream> // std::cout
#include <fstream>  // std::ifstream
/// \endcond

using namespace std;

EightInchDisk::EightInchDisk()
{
    // TODO Auto-generated constructor stub

}

EightInchDisk::EightInchDisk(const char*                       name,
                             SoftSectoredDisk::DiskImageFormat format): SoftSectoredDisk(name,
                                                                                         format)
{

}


EightInchDisk::~EightInchDisk()
{
    // TODO Auto-generated destructor stub
}

#if 0
bool
EightInchDisk::readRaw8(const char* name)
{
    // Currently just supporting the RAW HDOS 3.0 disk images... 40 track, single density,
    // single sided, 10 sectors/track, 256 bytes/sector.
    ifstream          file;
    unsigned long int fileSize;
    unsigned long int pos = 0;
    BYTE*             buf;

    file.open(name, ios::binary);

    if (!file.is_open())
    {
        debugss(ssFloppyDisk, ERROR, "%Unable to open file: %s\n", name);
        return false;
    }

    file.seekg(0, ios::end);
    fileSize = file.tellg();
    file.seekg(0, ios::beg);

    buf      = new BYTE[fileSize];
    file.read((char*) buf, fileSize);

    file.close();

    debugss(ssFloppyDisk, INFO, "RAW File: %s - Size: %lu\n", name, fileSize);

    switch (fileSize)
    {
        case 256256: // 1 side, 77 track, 26 sectors, 128 byte - FM
            break;

        case 512512: // 1 side, 77 track, 26 sectors, 256 byte - MFM
            // 1 side, 77 track, x sectors, 256 byte - FM
            break;
    }

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

#endif

#if 0
bool
EightInchDisk::readData(BYTE         side,
                        BYTE         track,
                        unsigned int pos,
                        BYTE&        data)
{

    return false;
}

bool
EightInchDisk::writeData(BYTE         side,
                         BYTE         track,
                         unsigned int pos,
                         BYTE         data)
{

    return false;
}

void
EightInchDisk::getControlInfo(unsigned int pos,
                              bool&        hole,
                              bool&        writeProtect)
{

}

void
EightInchDisk::setWriteProtect(bool value)
{

}

bool
EightInchDisk::checkWriteProtect(void)
{

    return true;
}

bool
EightInchDisk::readSectorData(BYTE         side,
                              BYTE         track,
                              BYTE         sector,
                              unsigned int pos,
                              BYTE&        data)
{

    return false;
}


void
EightInchDisk::eject(const char* name)
{

}


void
EightInchDisk::dump(void)
{
    SoftSectoredDisk::dump();
}
#endif
#endif
