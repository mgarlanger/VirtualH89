#if defined(__GUIwx__)
/////////////////////////////////////////////////////////////////////////////
// Name:        VirtualH89Frame.h
// Purpose:     
// Author:      
// Modified by: 
// Created:     Sat 22 Apr 2017 17:02:50 CDT
// RCS-ID:      
// Copyright:   
// Licence:     
/////////////////////////////////////////////////////////////////////////////

#ifndef _VIRTUALH89FRAME_H_
#define _VIRTUALH89FRAME_H_


/*!
 * Includes
 */

////@begin includes
#include "wx/frame.h"
////@end includes

#include <wx/bitmap.h>
#include <wx/timer.h>

#include <vector>

/*!
 * Forward declarations
 */

////@begin forward declarations
class H19TextFrame;
////@end forward declarations

/*!
 * Control identifiers
 */

////@begin control identifiers
#define ID_VIRTUALH89FRAME 10000
#define ID_MENUITEM_FileExit 10002
#define ID_MENUITEM_ViewConfig 10004
#define ID_MENUITEM_HelpAbout 10003
#define SYMBOL_VIRTUALH89FRAME_STYLE wxCAPTION|wxSYSTEM_MENU|wxMINIMIZE_BOX|wxCLOSE_BOX
#define SYMBOL_VIRTUALH89FRAME_TITLE _("Virtual Heathkit H-89 All-in-One Computer")
#define SYMBOL_VIRTUALH89FRAME_IDNAME ID_VIRTUALH89FRAME
#define SYMBOL_VIRTUALH89FRAME_SIZE wxSize(400, 300)
#define SYMBOL_VIRTUALH89FRAME_POSITION wxDefaultPosition
////@end control identifiers

/*!
 * VirtualH89Frame class declaration
 */

class VirtualH89Frame: public wxFrame
{    
    DECLARE_CLASS( VirtualH89Frame )
    DECLARE_EVENT_TABLE()

public:
    /// Constructors
    VirtualH89Frame();
    VirtualH89Frame( wxWindow* parent, wxWindowID id = SYMBOL_VIRTUALH89FRAME_IDNAME, const wxString& caption = SYMBOL_VIRTUALH89FRAME_TITLE, const wxPoint& pos = SYMBOL_VIRTUALH89FRAME_POSITION, const wxSize& size = SYMBOL_VIRTUALH89FRAME_SIZE, long style = SYMBOL_VIRTUALH89FRAME_STYLE );

    bool Create( wxWindow* parent, wxWindowID id = SYMBOL_VIRTUALH89FRAME_IDNAME, const wxString& caption = SYMBOL_VIRTUALH89FRAME_TITLE, const wxPoint& pos = SYMBOL_VIRTUALH89FRAME_POSITION, const wxSize& size = SYMBOL_VIRTUALH89FRAME_SIZE, long style = SYMBOL_VIRTUALH89FRAME_STYLE );

    /// Destructor
    ~VirtualH89Frame();

    /// Initialises member variables
    void Init();

    /// Creates the controls and sizers
    void CreateControls();

////@begin VirtualH89Frame event handler declarations

    /// wxEVT_COMMAND_MENU_SELECTED event handler for ID_MENUITEM_FileExit
    void OnMENUITEMFileExitClick( wxCommandEvent& event );

    /// wxEVT_COMMAND_MENU_SELECTED event handler for ID_MENUITEM_HelpAbout
    void OnMENUITEMHelpAboutClick( wxCommandEvent& event );

////@end VirtualH89Frame event handler declarations

////@begin VirtualH89Frame member function declarations

    /// Retrieves bitmap resources
    wxBitmap GetBitmapResource( const wxString& name );

    /// Retrieves icon resources
    wxIcon GetIconResource( const wxString& name );
////@end VirtualH89Frame member function declarations

    /// Should we show tooltips?
    static bool ShowToolTips();

////@begin VirtualH89Frame member variables
    H19TextFrame* m_H19TextFrame;
////@end VirtualH89Frame member variables
};

extern VirtualH89Frame *TheFrame;
extern std::vector<wxBitmap> FontBitmaps;

#endif
    // _VIRTUALH89FRAME_H_
#endif
