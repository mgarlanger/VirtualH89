#if defined(__GUIglut__) || !defined(__GUIwx__)
///
/// \name GUIglut.h
///
/// A GUI implementation based on glut.
///
/// \date Apr 20, 2017
/// \author David Troendle and Mark Garlanger
///

#ifndef GUIGLUT_H_
#define GUIGLUT_H_

#include "GUI.h"
#include "h19.h"

/// \cond
#include <pthread.h>
#include <unistd.h>
#include <assert.h>

#ifdef __APPLE__
#include <OpenGL/glu.h>
#include <GLUT/glut.h>
#else
#include <GL/glut.h>
#endif
/// \endcond

typedef void (* tGLUTKeyboardFunc)(unsigned char Key, int x, int y);

class GUIglut: public GUI
{
  public:
    GUIglut();
    virtual ~GUIglut() override;

    // Interface functions.
    virtual void GUIDisplay(void) override;
    virtual void InitGUI(void) override;
    virtual void StartGUI(void) override;

    // Callback functions.
    virtual void SetKeyboardFunc(tKeyboardFunc KeyboardFunc) override;
    virtual void SetDisplayFunc(tDisplayFunc DisplayFunc) override;
    virtual void SetTimerFunc(unsigned int ms, tTimerFunc TimerFunc) override;

  private:
    static void GLUTTimerFunc(int i);
    static tDisplayFunc  GUITimerFunc;
    static unsigned int  m_ms;

    static void GLUTDisplayFunc(void);
    static tDisplayFunc  GUIDisplayFunc;

    static void GLUTKeyboardFunc(unsigned char Key, int x, int y);
    static tKeyboardFunc GUIKeyboardFunc;

    static void reshape(int w,
                        int h);

    static void special(int key,
                        int x,
                        int y);

    unsigned int   fontOffset_m;

    unsigned char* fontTable;

};

#endif /* GUIGLUT_H_ */
#endif
