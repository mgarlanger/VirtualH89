#if defined(__GUIwx__)
/////////////////////////////////////////////////////////////////////////////
// Name:        H19TextFrame.cpp
// Purpose:     
// Author:      
// Modified by: 
// Created:     Sun 23 Apr 2017 06:59:01 CDT
// RCS-ID:      
// Copyright:   
// Licence:     
/////////////////////////////////////////////////////////////////////////////

// For compilers that support precompilation, includes "wx/wx.h".
#include "wx/wxprec.h"

#ifdef __BORLANDC__
#pragma hdrstop
#endif

#ifndef WX_PRECOMP
#include "wx/wx.h"
#endif

////@begin includes
////@end includes

#include "H19TextFrame.h"

////@begin XPM images
////@end XPM images


/*
 * H19TextFrame type definition
 */

IMPLEMENT_DYNAMIC_CLASS( H19TextFrame, wxScrolledWindow )


/*
 * H19TextFrame event table definition
 */

BEGIN_EVENT_TABLE( H19TextFrame, wxScrolledWindow )

////@begin H19TextFrame event table entries
    EVT_PAINT( H19TextFrame::OnPaint )
    EVT_CHAR( H19TextFrame::OnChar )
////@end H19TextFrame event table entries

END_EVENT_TABLE()

#include "VirtualH89Frame.h"
#include "h19.h"

#include "GUIwxWidgets.h"

extern GUIwxWidgets *TheGUIwxWidgets;

/*
 * H19TextFrame constructors
 */

H19TextFrame::H19TextFrame()
{
    Init();
}

H19TextFrame::H19TextFrame(wxWindow* parent, wxWindowID id, const wxPoint& pos, const wxSize& size, long style)
{
    Init();
    Create(parent, id, pos, size, style);
}


/*
 * H19TextFrame creator
 */

bool H19TextFrame::Create(wxWindow* parent, wxWindowID id, const wxPoint& pos, const wxSize& size, long style)
{
////@begin H19TextFrame creation
    wxScrolledWindow::Create(parent, id, pos, size, style);
    CreateControls();
////@end H19TextFrame creation
    return true;
}


/*
 * H19TextFrame destructor
 */

H19TextFrame::~H19TextFrame()
{
////@begin H19TextFrame destruction
////@end H19TextFrame destruction
}


/*
 * Member initialisation
 */

void H19TextFrame::Init()
{
////@begin H19TextFrame member initialisation
////@end H19TextFrame member initialisation
}


/*
 * Control creation for H19TextFrame
 */

void H19TextFrame::CreateControls()
{    
////@begin H19TextFrame content construction
    H19TextFrame* itemScrolledWindow1 = this;

    this->SetForegroundColour(wxColour(255, 255, 255));
    this->SetBackgroundColour(wxColour(0, 0, 0));
    this->SetScrollbars(1, 1, 0, 0);
////@end H19TextFrame content construction
}


/*
 * wxEVT_PAINT event handler for ID_SCROLLEDWINDOW_H19Text
 */

void H19TextFrame::OnPaint( wxPaintEvent& event )
{
////@begin wxEVT_PAINT event handler for ID_SCROLLEDWINDOW_H19Text in H19TextFrame.
    // Before editing this code, remove the block markers.
    wxPaintDC dc(this);
////@end wxEVT_PAINT event handler for ID_SCROLLEDWINDOW_H19Text in H19TextFrame. 

    // Initial paint will not have the emulator running.  Check for that.
    if (H19::GetH19())
    {
      // Draw the character using the font bitmaps.
      for (auto y = 0u; y < H19Screen::rows_c; ++y)
      {
        for (auto x = 0u; x < H19Screen::cols_c; ++x)
        {
          unsigned int Char = H19::GetH19()->screen_m[x][y] & 0xFFu;
          // Chars above 0x80 are inverse video of the ASCII char.
          // Get ASCII char.
          unsigned int ASCIIChar = Char & 0x7Fu;
          if ((ASCIIChar >= ' ') && (ASCIIChar < 0x7Fu))
          {
            // Draw the character at the correct location using the bitmap
            // for that character.  The bitmaps were created in VirtualH89Frame::CreateControl().
            dc.DrawBitmap(FontBitmaps[Char], x * 8u, y * 20u);
          }
        }
      }
    }

    return;
}

/*
 * Should we show tooltips?
 */

bool H19TextFrame::ShowToolTips()
{
    return true;
}

/*
 * Get bitmap resources
 */

wxBitmap H19TextFrame::GetBitmapResource( const wxString& name )
{
    // Bitmap retrieval
////@begin H19TextFrame bitmap retrieval
    wxUnusedVar(name);
    return wxNullBitmap;
////@end H19TextFrame bitmap retrieval
}

/*
 * Get icon resources
 */

wxIcon H19TextFrame::GetIconResource( const wxString& name )
{
    // Icon retrieval
////@begin H19TextFrame icon retrieval
    wxUnusedVar(name);
    return wxNullIcon;
////@end H19TextFrame icon retrieval
}


/*
 * wxEVT_CHAR event handler for ID_SCROLLEDWINDOW_H19Text
 */

void H19TextFrame::OnChar( wxKeyEvent& event )
{
////@begin wxEVT_CHAR event handler for ID_SCROLLEDWINDOW_H19Text in H19TextFrame.
    // Before editing this code, remove the block markers.
    event.Skip();
////@end wxEVT_CHAR event handler for ID_SCROLLEDWINDOW_H19Text in H19TextFrame. 

    if (TheGUIwxWidgets->GUIKeyboardFunc)
    {
      int Key = event.GetKeyCode();
      switch (Key)
      {
        case WXK_F1:
          TheGUIwxWidgets->GUIKeyboardFunc('S' | 0x80);
          break;

        case WXK_F2:
          TheGUIwxWidgets->GUIKeyboardFunc('T' | 0x80);
          break;

        case WXK_F3:
          TheGUIwxWidgets->GUIKeyboardFunc('U' | 0x80);
          break;

        case WXK_F4:
          TheGUIwxWidgets->GUIKeyboardFunc('V' | 0x80);
          break;

        case WXK_F5:
          TheGUIwxWidgets->GUIKeyboardFunc('W' | 0x80);
          break;

        case WXK_F6:
          TheGUIwxWidgets->GUIKeyboardFunc('P' | 0x80);
          break;

        case WXK_F7:
          TheGUIwxWidgets->GUIKeyboardFunc('Q' | 0x80);
          break;

        case WXK_F8:
          TheGUIwxWidgets->GUIKeyboardFunc('R' | 0x80);
          break;

        case WXK_HOME:
          TheGUIwxWidgets->GUIKeyboardFunc('H' | 0x80);
          break;

        case WXK_UP:
          TheGUIwxWidgets->GUIKeyboardFunc('A' | 0x80);
          break;

        case WXK_DOWN:
          TheGUIwxWidgets->GUIKeyboardFunc('B' | 0x80);
          break;

        case WXK_LEFT:
          TheGUIwxWidgets->GUIKeyboardFunc('D' | 0x80);
          break;

        case WXK_RIGHT:
          TheGUIwxWidgets->GUIKeyboardFunc('C' | 0x80);
          break;

        default:
          TheGUIwxWidgets->GUIKeyboardFunc(Key);
          break;
      }
    }

    return;
}
#endif
