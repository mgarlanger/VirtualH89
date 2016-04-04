///
/// \name ParallelLink.cpp
///
///
/// \date Aug 12, 2013
/// \author Mark Garlanger
///

#include "ParallelLink.h"

#include "ParallelPortConnection.h"
#include "logger.h"

ParallelLink::ParallelLink(): data_m(0xff),
                              dataFromHost_m(false),
                              dataFromDrive_m(false),
                              busy_m(false),
                              DTR_m(false),
                              MRST_m(false),
                              DDOut_m(false),
                              DTAK_m(false),
                              error_m(false),
                              host_m(0),
                              device_m(0)
{


}

ParallelLink::~ParallelLink()
{

}

void
ParallelLink::sendHostData(BYTE data)
{
    debugss(ssParallel, INFO, "Entering: %d\n", data);

    if (host_m)
    {
        if ((dataFromDrive_m) || (dataFromHost_m))
        {
            debugss(ssParallel, ERROR, "Data already on bus: fromDrive: %d fromHost: %d.\n",
                    dataFromDrive_m, dataFromHost_m);
        }

        data_m          = data;
        dataFromDrive_m = true;

        setDTR(true);
    }
    else
    {
        debugss(ssParallel, ERROR, "Host not connected.\n");
    }
}

void
ParallelLink::sendDriveData(BYTE data)
{
    debugss(ssParallel, INFO, "Entering: %d\n", data);

    if (device_m)
    {
        if ((dataFromDrive_m) || (dataFromHost_m))
        {
            debugss(ssParallel, ERROR, "Data already on bus: fromDrive: %d fromHost: %d.\n",
                    dataFromDrive_m, dataFromHost_m);
        }

        data_m         = data;
        dataFromHost_m = true;
        setDTAK(true);
    }
    else
    {
        debugss(ssParallel, FATAL, "Drive not connected.\n");
    }
}

void
ParallelLink::readDataBusByHost(BYTE& data)
{
    debugss(ssParallel, INFO, "Entering, returning: 0x%02x\n", data_m);

    if (dataFromDrive_m)
    {
        data            = data_m;
        dataFromDrive_m = false;
    }
    else
    {
        debugss(ssParallel, FATAL, "Drive did not provide data: %d\n", dataFromHost_m);
    }
}


void
ParallelLink::readDataBusByDrive(BYTE& data)
{
    debugss(ssParallel, INFO, "Entering, returning: 0x%02x\n", data_m);

    if (dataFromHost_m)
    {
        data           = data_m;
        dataFromHost_m = false;
    }
    else
    {
        debugss(ssParallel, FATAL, "Drive did not provide data: %d\n", dataFromDrive_m);
    }
}



void
ParallelLink::setBusy(bool val)
{
    debugss(ssParallel, INFO, "Entering\n");

    if (host_m)
    {
        if (val)
        {
            host_m->raiseSignal(ParallelPortConnection::st_Busy);
        }
        else
        {
            host_m->lowerSignal(ParallelPortConnection::st_Busy);
        }
    }

    busy_m = val;
}

bool
ParallelLink::readBusy()
{
    debugss(ssParallel, INFO, "Entering\n");
    return busy_m;
}

void
ParallelLink::setDTR(bool val)
{
    debugss(ssParallel, INFO, "Entering\n");

    if (host_m)
    {
        if (val)
        {
            host_m->raiseSignal(ParallelPortConnection::st_DTR);
        }
        else
        {
            host_m->lowerSignal(ParallelPortConnection::st_DTR);
        }
    }
    else
    {
        debugss(ssParallel, ERROR, "host_m is NULL\n");

    }

    DTR_m = val;
}

bool
ParallelLink::readDTR()
{
    debugss(ssParallel, INFO, "Entering\n");
    return DTR_m;
}

void
ParallelLink::masterReset()
{
    debugss(ssParallel, INFO, "Entering\n");

    if (device_m)
    {
        device_m->raiseSignal(ParallelPortConnection::st_MasterReset);
    }
}

void
ParallelLink::setDDOut(bool val)
{
    debugss(ssParallel, INFO, "Entering\n");

    if (host_m)
    {
        if (val)
        {
            host_m->raiseSignal(ParallelPortConnection::st_DDOUT);
        }
        else
        {
            host_m->lowerSignal(ParallelPortConnection::st_DDOUT);
        }

        //    host_m->notify(ParallelPortConnection::nt_DDOUT);
    }

    DDOut_m = val;
}

bool
ParallelLink::readDDOut()
{
    debugss(ssParallel, INFO, "Entering\n");
    return DDOut_m;
}


void
ParallelLink::setDTAK(bool val)
{
    debugss(ssParallel, INFO, "Entering\n");

    if (device_m)
    {
        if (val)
        {
            device_m->raiseSignal(ParallelPortConnection::st_DTAK);
        }
        else
        {
            device_m->lowerSignal(ParallelPortConnection::st_DTAK);
        }
    }

    DTAK_m = val;
}

bool
ParallelLink::readDTAK()
{
    debugss(ssParallel, INFO, "Entering\n");
    return DTAK_m;
}

void
ParallelLink::setError(bool val)
{
    debugss(ssParallel, INFO, "Entering\n");

    if (host_m)
    {
        if (val)
        {
            host_m->raiseSignal(ParallelPortConnection::st_Error);
        }
        else
        {
            host_m->lowerSignal(ParallelPortConnection::st_Error);
        }
    }

    error_m = val;
}

bool
ParallelLink::readError()
{
    debugss(ssParallel, INFO, "Entering\n");
    return error_m;
}

void
ParallelLink::registerDevice(ParallelPortConnection* device)
{
    debugss(ssParallel, INFO, "Entering\n");

    if (device_m)
    {
        debugss(ssParallel, ERROR, "device already defined\n");
    }

    device_m = device;
}

void
ParallelLink::registerHost(ParallelPortConnection* host)
{
    debugss(ssParallel, INFO, "Entering\n");

    if (host_m)
    {
        debugss(ssParallel, ERROR, "device already defined\n");
    }

    host_m = host;
}
