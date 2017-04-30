///
/// \name GUI.h
///
/// GUI base class.  It defines the interface between the H19 terminal class and an abstract GUI.
/// It also houses the character generator ROM, which is needed by any GUI implementation.
///
/// \date Apr 20, 2017
/// \author David Troendle and Mark Garlanger
///

#include <cassert>

#include "GUI.h"

#include "h19-font.h"

// All GUI access is through this pointer to the base GUI class.
GUI* TheGUI = nullptr;

GUI::GUI()
{
    // There can only be one GUI engine in the system.
    assert(TheGUI == nullptr);

    // This is not the one and only GUI enging.
    TheGUI = this;

    return;
}

GUI::~GUI()
{
    // We no longer have a GUI engine.
    TheGUI = nullptr;

    return;
}
