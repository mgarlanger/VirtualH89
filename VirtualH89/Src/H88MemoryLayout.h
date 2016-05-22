///
/// \file H88MemoryLayout.h
///
/// H88 48K RAM+ROM layout
///
/// \date Mar 05, 2016
/// \author Douglas Miller
///
#ifndef H88MEMORYLAYOUT_H_
#define H88MEMORYLAYOUT_H_

#include "MemoryLayout.h"

class Memory8K;

class H88MemoryLayout: public MemoryLayout
{
  public:
    H88MemoryLayout(shared_ptr<Memory8K> hdos);
    H88MemoryLayout(shared_ptr<MemoryLayout> nonOrg0Layout);
};

#endif // H88MEMORYLAYOUT_H_
