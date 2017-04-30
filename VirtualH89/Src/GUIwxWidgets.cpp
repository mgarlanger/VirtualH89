#if defined(__GUIwx__)
///
/// \name GUIglut.h
///
/// A GUI implementation based on glut.
///
/// \date Apr 20, 2017
/// \author David Troendle and Mark Garlanger
///

#include <cassert>
#include "GUIwxWidgets.h"
#include "VirtualH89Frame.h"

tKeyboardFunc GUIwxWidgets::GUIKeyboardFunc = nullptr;
tDisplayFunc  GUIwxWidgets::GUIDisplayFunc;
tDisplayFunc  GUIwxWidgets::GUITimerFunc;

unsigned int  GUIwxWidgets::m_ms;

extern VirtualH89Frame *TheFrame;

wxThread::ExitCode wxGUIThread::Entry(void)
{
  // Loop until kill by GUIwxWIdgets destructor.
  for (;;)
  {
    Sleep(Period);
    GUITimerFunc();

    // If the screen changed we need to update it.
    if (H19::GetH19()->checkUpdated())
    {
      TheFrame->Refresh();
    }
  }

  return 0;
}

GUIwxWidgets::GUIwxWidgets()
{
    fontTable = (unsigned char*) fontTableForward;

    return;
}

GUIwxWidgets::~GUIwxWidgets()
{
  if (Thread)
  {
    Thread->Kill();
    delete Thread;
    Thread = nullptr;
  }
}

void
GUIwxWidgets::GUIDisplay(void)
{
    TheFrame->Refresh();

    return;
}

void
GUIwxWidgets::InitGUI(void)
{
    assert(H19::GetH19());


    return;
}

void
GUIwxWidgets::StartGUI(void)
{
//    glutMainLoop();

    return;
}

void
GUIwxWidgets::SetTimerFunc(unsigned int ms, tTimerFunc TimerFunc)
{
    assert(TimerFunc);

    m_ms         = ms;
    GUITimerFunc = TimerFunc;

    if (TimerFunc)
    {
      Thread = new wxGUIThread(TimerFunc, ms);
      Thread->Run();
    }

    return;

}

void
GUIwxWidgets::SetDisplayFunc(tDisplayFunc DisplayFunc)
{
    assert(DisplayFunc);

    GUIDisplayFunc = DisplayFunc;

    return;
}

void
GUIwxWidgets::SetKeyboardFunc(tKeyboardFunc KeyboardFunc)
{
    assert(KeyboardFunc);

    GUIKeyboardFunc = KeyboardFunc;

    return;
}

void
GUIwxWidgets::GUIwxWidgetsDisplayFunc(void)
{
    assert(GUIDisplayFunc);
    GUIDisplayFunc();

    return;
}
#endif
