///
///  \file Memory8K.cpp
///
///  \author  Mark Garlanger
///  \date    May 8, 2016
///

#include "Memory8K.h"

#include <string.h>


Memory8K::Memory8K(WORD base): base_m(base)
{

    memset(mem, 0, sizeof(mem));
}
