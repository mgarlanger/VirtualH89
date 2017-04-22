//
//  DiskSide.cpp
//  VirtualH89
//
//  Created by Mark Garlanger on 6/27/16.
//  Copyright © 2016 Mark Garlanger. All rights reserved.
//

#include "DiskSide.h"

#include "Track.h"
#include "Sector.h"
#include "logger.h"

using namespace std;

DiskSide::DiskSide(BYTE sideNum): sideNum_m(sideNum)
{
    debugss(ssFloppyDisk, INFO, "%s: sideNum = %d\n", __FUNCTION__, sideNum);
    if (sideNum > 1)
    {
        debugss(ssFloppyDisk, ERROR, "%s: out of range - sideNum = %d\n", __FUNCTION__, sideNum);
    }
}

DiskSide::~DiskSide()
{

}


bool
DiskSide::addTrack(std::shared_ptr<Track> track)
{
    unsigned long curSize = trackData_m.size();
    // we expect the tracks in order, warn if that is not the case.
    if (track->getTrackNumber() != curSize)
    {
        debugss(ssFloppyDisk, WARNING, "%s: unexpected trackNum: %d curSize: %d\n",
                __FUNCTION__, track->getTrackNumber(), curSize);
    }
    // add it anyways
    trackData_m.push_back(track);

    return true;
}

std::shared_ptr<Sector>
DiskSide::findSector(BYTE trackNum, BYTE sectorNum)
{

    debugss(ssFloppyDisk, INFO, "findSector - track %d sector: %d\n", trackNum, sectorNum);

    if (trackNum >= trackData_m.size())
    {
        return nullptr;
    }
    shared_ptr<Track> track = trackData_m[trackNum];

    // Normally they should be in order, but nothing in the file format guarantees that
    if (track->getTrackNumber() != trackNum)
    {
        track = nullptr;

        // not found where expected, lets see if we can find the matching track
        for (shared_ptr<Track> testTrack : trackData_m)
        {
            if (testTrack->getTrackNumber() == trackNum)
            {
                debugss(ssFloppyDisk, INFO, "found\n");

                track = testTrack;
                break;
            }
        }

    }

    if (!track)
    {
        return nullptr;
    }


    return track->findSector(sectorNum);

}
