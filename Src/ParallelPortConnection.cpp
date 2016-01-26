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
    // TODO Auto-generated constructor stub

}

ParallelPortConnection::~ParallelPortConnection()
{
    // TODO Auto-generated destructor stub
}

void ParallelPortConnection::connectLink(ParallelLink *link)
{
    debugss(ssParallel, INFO, "%s: Entering\n", __FUNCTION__);
    if (link_m)
    {
        debugss(ssParallel, ERROR, "%s: Link already in use\n", __FUNCTION__);
    }

    link_m = link;
}
