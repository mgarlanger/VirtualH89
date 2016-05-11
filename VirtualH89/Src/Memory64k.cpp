///
///  \file Memory64k.cpp
///
///  \author  Mark Garlanger
///  \date    May 8, 2016
///

#include "Memory64K.h"

#include "RAMemory8K.h"

Memory64K::Memory64K() {

    // for 64K all base addresses are defined
    for (int x = 0; x < 8; ++x)
    {
        mem64k[x] = make_shared<RAMemory8K>(x << 13);
    }
}
