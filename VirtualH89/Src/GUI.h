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

typedef void (*tKeyboardFunc)(unsigned char Key);
typedef void (*tDisplayFunc)(void);
typedef void (*tTimerFunc)(void);

#include <iostream>

class GUI
{
public:
  GUI();
  virtual
  ~GUI();

  // Interface functions.
public:
  virtual void GUIDisplay(void) = 0;
  virtual void InitGUI(void) = 0;
  virtual void SetKeyboardFunc(tKeyboardFunc KeyboardFunc) = 0;
  virtual void SetDisplayFunc(tDisplayFunc KeyboardFunc) = 0;
  virtual void SetTimerFunc(unsigned int ms, tTimerFunc TimerFunc) = 0;
  virtual void StartGUI(void) = 0;
};

// All client GUI access is through this pointer to the base GUI class.
// Access is achieved by adding #include "GUI.h".  The include gives the access.

extern GUI *TheGUI;

#endif /* GUI_H_ */
