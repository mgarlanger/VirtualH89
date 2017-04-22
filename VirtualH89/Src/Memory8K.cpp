///
///  \file Memory8K.cpp
///
///  \author  Mark Garlanger
///  \date    May 8, 2016
///

#include "Memory8K.h"

/// \cond
#include <string.h>
/// \endcond


Memory8K::Memory8K(WORD base): base_m(base)
{

    memset(mem, 0, sizeof(mem));
}

Memory8K::~Memory8K()
{

}

WORD
Memory8K::getBase()
{
    return base_m;
}
