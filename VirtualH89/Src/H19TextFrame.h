#if defined(__GUIwx__)
/////////////////////////////////////////////////////////////////////////////
// Name:        H19TextFrame.h
// Purpose:     
// Author:      
// Modified by: 
// Created:     Sun 23 Apr 2017 06:59:01 CDT
// RCS-ID:      
// Copyright:   
// Licence:     
/////////////////////////////////////////////////////////////////////////////

#ifndef _H19TEXTFRAME_H_
#define _H19TEXTFRAME_H_


/*!
 * Includes
 */

////@begin includes
////@end includes

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
#define ID_SCROLLEDWINDOW_H19Text 10005
#define SYMBOL_H19TEXTFRAME_STYLE wxSUNKEN_BORDER
#define SYMBOL_H19TEXTFRAME_IDNAME ID_SCROLLEDWINDOW_H19Text
#define SYMBOL_H19TEXTFRAME_SIZE wxSize(640, 480)
#define SYMBOL_H19TEXTFRAME_POSITION wxDefaultPosition
////@end control identifiers


/*!
 * H19TextFrame class declaration
 */

class H19TextFrame: public wxScrolledWindow
{    
    DECLARE_DYNAMIC_CLASS( H19TextFrame )
    DECLARE_EVENT_TABLE()

public:
    /// Constructors
    H19TextFrame();
    H19TextFrame(wxWindow* parent, wxWindowID id = ID_SCROLLEDWINDOW_H19Text, const wxPoint& pos = wxDefaultPosition, const wxSize& size = wxSize(640, 480), long style = wxSUNKEN_BORDER);

    /// Creation
    bool Create(wxWindow* parent, wxWindowID id = ID_SCROLLEDWINDOW_H19Text, const wxPoint& pos = wxDefaultPosition, const wxSize& size = wxSize(640, 480), long style = wxSUNKEN_BORDER);

    /// Destructor
    ~H19TextFrame();

    /// Initialises member variables
    void Init();

    /// Creates the controls and sizers
    void CreateControls();

////@begin H19TextFrame event handler declarations

    /// wxEVT_PAINT event handler for ID_SCROLLEDWINDOW_H19Text
    void OnPaint( wxPaintEvent& event );

    /// wxEVT_CHAR event handler for ID_SCROLLEDWINDOW_H19Text
    void OnChar( wxKeyEvent& event );

////@end H19TextFrame event handler declarations

////@begin H19TextFrame member function declarations

    /// Retrieves bitmap resources
    wxBitmap GetBitmapResource( const wxString& name );

    /// Retrieves icon resources
    wxIcon GetIconResource( const wxString& name );
////@end H19TextFrame member function declarations

    /// Should we show tooltips?
    static bool ShowToolTips();

////@begin H19TextFrame member variables
////@end H19TextFrame member variables
};

#endif
    // _H19TEXTFRAME_H_
#endif
