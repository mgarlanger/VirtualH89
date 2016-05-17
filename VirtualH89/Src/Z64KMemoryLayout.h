///
/// \file Z64KMemoryLayout.h
///
/// 16K add-on RAM to a basic 48K H88 layout to create a 64K RAM layout.
///
/// \date Mar 05, 2016
/// \author Douglas Miller
///
#ifndef Z64KMEMORYLAYOUT_H_
#define Z64KMEMORYLAYOUT_H_

#include "MemoryLayout.h"

class Z64KMemoryLayout: public MemoryLayout
{
  public:
    Z64KMemoryLayout(shared_ptr<MemoryLayout> rom);
};

#endif // Z64KMEMORYLAYOUT_H_
