/// \file GenericFloppyDisk.cpp
///
/// \brief virtual Floppy Disk interface.
///
/// \date Feb 2, 2016
/// \author Douglas Miller
///

#include "GenericFloppyDisk.h"
#include "IMDFloppyDisk.h"
#include "TD0FloppyDisk.h"

#include "logger.h"

using namespace std;


GenericFloppyDisk::GenericFloppyDisk(): writeProtect_m(true),
                                        doubleDensity_m(false),
                                        numTracks_m(0),
                                        numSectors_m(0),
                                        numSides_m(0),
                                        secSize_m(0),
                                        mediaSize_m(0)/*,
                                                         imageName_m(nullptr),
                                                         curSector_m(nullptr),
                                                         dataPos_m(0),
                                                         sectorLength_m(0),
                                                         secLenCode_m(0),
                                                         hypoTrack_m(false),
                                                         hyperTrack_m(false),
                                                         ready_m(true)*/
{
}

GenericFloppyDisk::~GenericFloppyDisk()
{
}

GenericFloppyDisk::FileType_t
GenericFloppyDisk::determineFileType(std::string filename)
{
    size_t pos = filename.rfind('.');

    if (pos == string::npos)
    {
        return Unknown;
    }

    std::string extension = filename.substr(pos + 1);

    std::transform(extension.begin(), extension.end(), extension.begin(), ::tolower);

    if (extension == "imd")
    {
        return IMD;
    }
    if (extension == "td0")
    {
        return TD0;
    }
    return Unknown;
}


shared_ptr<GenericFloppyDisk>
GenericFloppyDisk::loadDiskImage(vector<string> argv)
{

    if (argv.size() < 1)
    {
        debugss(ssFloppyDisk, WARNING, "no file specified\n");
        return nullptr;
    }

    string     filename(argv[0]);

    FileType_t fileType = determineFileType(filename);

    {
        switch (fileType)
        {
            case IMD:
                return IMDFloppyDisk::getDiskette(argv);

            case TD0:
                return TD0FloppyDisk::getDiskette(argv);

            case Unknown:
                return nullptr;
        }
    }
}


void
GenericFloppyDisk::setDriveType(BYTE numTracks)
{
    if (numTracks == numTracks_m)
    {
        hypoTrack_m = hyperTrack_m = false;
    }
    else if ((numTracks == 40) && (numTracks_m == 80))
    {
        hyperTrack_m = true;
        hypoTrack_m  = false;
    }
    else if ((numTracks == 80) && (numTracks_m == 40))
    {
        hyperTrack_m = false;
        hypoTrack_m  = true;
    }
}
