///
/// \name ParallelPortConnection.cpp
///
///
/// \date Aug 17, 2013
/// \author Mark Garlanger
///

#include "ParallelPortConnection.h"

#include "logger.h"

ParallelPortConnection::ParallelPortConnection(): link_m(0)
{


}

ParallelPortConnection::~ParallelPortConnection()
{

}

void
ParallelPortConnection::connectLink(ParallelLink* link)
{
    debugss(ssParallel, INFO, "Entering\n");

    if (link_m)
    {
        debugss(ssParallel, ERROR, "Link already in use\n");
    }

    link_m = link;
}
