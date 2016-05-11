///
/// \file Memory64K.h
///
/// A convenience container for 8 8K pages of memory, addressed 0000-E000.
///
/// \date Mar 05, 2016
/// \author Douglas Miller
///
#ifndef MEMORY64K_H_
#define MEMORY64K_H_

#include "h89Types.h"

#include <memory>

class Memory8K;

using namespace std;

class Memory64K
{
  public:
    Memory64K();

    shared_ptr<Memory8K> getPage(BYTE page)
    {
        // \todo - error if out of range
        return mem64k[page & 0x7];
    }

  private:
    shared_ptr<Memory8K> mem64k[8];
};

#endif // MEMORY64K_H_
