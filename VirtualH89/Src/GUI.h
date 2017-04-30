///
/// \name GUI.h
///
/// GUI base class.  It defines the interface between the H19 terminal class and an abstract GUI.
/// It also houses the character generator ROM, which is needed by any GUI implementation.
///
/// \date Apr 20, 2017
/// \author David Troendle and Mark Garlanger
///

#ifndef GUI_H_
#define GUI_H_

// Ensure exactly one GUI defined.
#undef __GUIDefined__
#if defined(__GUIglut__)
#define __GUIDefined__
#endif

#if defined(__GUIwx__) && defined(__GUIDefined__)
#error Configured for more than 1 GUI.  Choices are -D__GUIglut__ and -D__GUIwx__.  Please choose exactly 1.
#elif defined(__GUIwx__)
#define __GUIDefined__
#endif

// More checks as other GUIs added go here>
//#if defined(__GUI??__) && defined(__GUIDefined__)
//#error Configured for more than 1 GUI.  Choices are -D__GUIglut__, -D__GUIwx__, ....  Please choose exactly 1.
//#elif defined(__GUI??__)
//#define __GUIDefined__
//#endif

typedef void (* tKeyboardFunc)(unsigned char Key);
typedef void (* tDisplayFunc)(void);
typedef void (* tTimerFunc)(void);

#include <iostream>

class GUI
{
  public:
    GUI();
    virtual ~GUI();

    // Interface functions.
  public:
    virtual void GUIDisplay(void)                                    = 0;
    virtual void InitGUI(void)                                       = 0;
    virtual void SetKeyboardFunc(tKeyboardFunc KeyboardFunc)         = 0;
    virtual void SetDisplayFunc(tDisplayFunc KeyboardFunc)           = 0;
    virtual void SetTimerFunc(unsigned int ms, tTimerFunc TimerFunc) = 0;
    virtual void StartGUI(void)                                      = 0;
};

// All client GUI access is through this pointer to the base GUI class.
// Access is achieved by adding #include "GUI.h".  The include gives the access.

extern GUI* TheGUI;
extern const unsigned char fontTableForward[];
extern const unsigned char fontTableInverted[];

#endif /* GUI_H_ */
