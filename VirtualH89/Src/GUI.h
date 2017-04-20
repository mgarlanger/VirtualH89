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
  inline virtual void InitGUI(void) {IncorrectClassDerivation(); return;}
  inline virtual void SetKeyboardFunc(tKeyboardFunc KeyboardFunc) {IncorrectClassDerivation(); return;}
  inline virtual void SetDisplayFunc(tDisplayFunc KeyboardFunc) {IncorrectClassDerivation(); return;}
  inline virtual void SetTimerFunc(unsigned int ms, tTimerFunc TimerFunc) {IncorrectClassDerivation(); return;}

private:
  inline void IncorrectClassDerivation(void) {std::cerr << "GUI class not derived correctly." << std::endl; exit(1); return;}
};

// All client GUI access is through this pointer to the base GUI class.
// Access is achieved by adding #include "GUI.h".  Including a GUI instance does this.

extern GUI *TheGUI;

#endif /* GUI_H_ */
