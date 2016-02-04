/// \file GenericFloppyDisk.cpp
///
/// \date Feb 2, 2016
/// \author Douglas Miller
///

#include "GenericFloppyDisk.h"


GenericFloppyDisk::GenericFloppyDisk():
    writeProtect_m(true),
    doubleDensity_m(false),
    numTracks_m(0),
    numSectors_m(0),
    numSides_m(0),
    secSize_m(0),
    mediaSize_m(0)
{
}

GenericFloppyDisk::~GenericFloppyDisk()
{
}
