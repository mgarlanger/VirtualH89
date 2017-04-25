#if defined(__GUIwx__)
/////////////////////////////////////////////////////////////////////////////
// Name:        AboutVirtualH89.h
// Purpose:     
// Author:      
// Modified by: 
// Created:     Sat 22 Apr 2017 17:10:14 CDT
// RCS-ID:      
// Copyright:   
// Licence:     
/////////////////////////////////////////////////////////////////////////////

#ifndef _ABOUTVIRTUALH89_H_
#define _ABOUTVIRTUALH89_H_


/*!
 * Includes
 */

////@begin includes
////@end includes

/*!
 * Forward declarations
 */

////@begin forward declarations
////@end forward declarations

/*!
 * Control identifiers
 */

////@begin control identifiers
#define ID_ABOUTVIRTUALH89 10000
#define ID_TEXTCTRL_AboutText 10001
#define SYMBOL_ABOUTVIRTUALH89_STYLE wxCAPTION|wxSYSTEM_MENU|wxCLOSE_BOX|wxTAB_TRAVERSAL
#define SYMBOL_ABOUTVIRTUALH89_TITLE _("About Virtual Heathkit H-89 All-in-One Computer")
#define SYMBOL_ABOUTVIRTUALH89_IDNAME ID_ABOUTVIRTUALH89
#define SYMBOL_ABOUTVIRTUALH89_SIZE wxDefaultSize
#define SYMBOL_ABOUTVIRTUALH89_POSITION wxDefaultPosition
////@end control identifiers


/*!
 * AboutVirtualH89 class declaration
 */

class AboutVirtualH89: public wxDialog
{    
    DECLARE_DYNAMIC_CLASS( AboutVirtualH89 )
    DECLARE_EVENT_TABLE()

public:
    /// Constructors
    AboutVirtualH89();
    AboutVirtualH89( wxWindow* parent, wxWindowID id = SYMBOL_ABOUTVIRTUALH89_IDNAME, const wxString& caption = SYMBOL_ABOUTVIRTUALH89_TITLE, const wxPoint& pos = SYMBOL_ABOUTVIRTUALH89_POSITION, const wxSize& size = SYMBOL_ABOUTVIRTUALH89_SIZE, long style = SYMBOL_ABOUTVIRTUALH89_STYLE );

    /// Creation
    bool Create( wxWindow* parent, wxWindowID id = SYMBOL_ABOUTVIRTUALH89_IDNAME, const wxString& caption = SYMBOL_ABOUTVIRTUALH89_TITLE, const wxPoint& pos = SYMBOL_ABOUTVIRTUALH89_POSITION, const wxSize& size = SYMBOL_ABOUTVIRTUALH89_SIZE, long style = SYMBOL_ABOUTVIRTUALH89_STYLE );

    /// Destructor
    ~AboutVirtualH89();

    /// Initialises member variables
    void Init();

    /// Creates the controls and sizers
    void CreateControls();

////@begin AboutVirtualH89 event handler declarations

////@end AboutVirtualH89 event handler declarations

////@begin AboutVirtualH89 member function declarations

    /// Retrieves bitmap resources
    wxBitmap GetBitmapResource( const wxString& name );

    /// Retrieves icon resources
    wxIcon GetIconResource( const wxString& name );
////@end AboutVirtualH89 member function declarations

    /// Should we show tooltips?
    static bool ShowToolTips();

////@begin AboutVirtualH89 member variables
////@end AboutVirtualH89 member variables
};

#endif
    // _ABOUTVIRTUALH89_H_
#endif
