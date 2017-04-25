#if defined(__GUIwx__)
///
/// \name GUIwxWidgets.h
///
/// A GUI implementation based on wxWidgets.
///
/// \date Apr 20, 2017
/// \author David Troendle and Mark Garlanger
///

#ifndef GUIWXWIDGETS_H_
#define GUIWXWIDGETS_H_

#include <wx/panel.h>
#include <wx/thread.h>

#include "GUI.h"
#include "h19.h"

/// \cond
#include <pthread.h>
#include <unistd.h>
#include <assert.h>
/// \cond

class wxGUIThread : public wxThread
{
public:
  wxGUIThread(tDisplayFunc  _GUITimerFunc, unsigned int _Period)
        : wxThread(wxTHREAD_DETACHED), GUITimerFunc(_GUITimerFunc), Period(_Period)
        { return; }
    ~wxGUIThread(void) { return;}

protected:
    ExitCode Entry(void);
    tDisplayFunc  GUITimerFunc;
    unsigned int  Period;
};

class GUIwxWidgets: public GUI
{
  public:
    GUIwxWidgets();
    virtual ~GUIwxWidgets() override;

    // Interface functions.
    virtual void GUIDisplay(void) override;
    virtual void InitGUI(void) override;
    virtual void StartGUI(void) override;

    // Callback functions.
    virtual void SetKeyboardFunc(tKeyboardFunc KeyboardFunc) override;
    virtual void SetDisplayFunc(tDisplayFunc DisplayFunc) override;
    virtual void SetTimerFunc(unsigned int ms, tTimerFunc TimerFunc) override;

    static tKeyboardFunc GUIKeyboardFunc;
    static tDisplayFunc  GUITimerFunc;

  private:
    static unsigned int  m_ms;

    static void GUIwxWidgetsDisplayFunc(void);
    static tDisplayFunc  GUIDisplayFunc;


    unsigned int   fontOffset_m;

    unsigned char* fontTable;

    wxGUIThread *Thread;

};

#endif
#endif
