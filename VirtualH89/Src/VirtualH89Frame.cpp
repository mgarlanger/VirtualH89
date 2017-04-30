#if defined(__GUIwx__)
/////////////////////////////////////////////////////////////////////////////
// Name:        VirtualH89Frame.cpp
// Purpose:     
// Author:      
// Modified by: 
// Created:     Sat 22 Apr 2017 17:02:50 CDT
// RCS-ID:      
// Copyright:   
// Licence:     
/////////////////////////////////////////////////////////////////////////////

// For compilers that support precompilation, includes "wx/wx.h".
#include "wx/wxprec.h"
#include <wx/image.h>

#include <bitset>

#ifdef __BORLANDC__
#pragma hdrstop
#endif

#ifndef WX_PRECOMP
#include "wx/wx.h"
#endif

////@begin includes
#include "H19TextFrame.h"
////@end includes

#include "VirtualH89Frame.h"

#include "AboutVirtualH89.h"

#include "GUIwxWidgets.h"

////@begin XPM images
////@end XPM images


/*
 * VirtualH89Frame type definition
 */

IMPLEMENT_CLASS( VirtualH89Frame, wxFrame )


/*
 * VirtualH89Frame event table definition
 */

BEGIN_EVENT_TABLE( VirtualH89Frame, wxFrame )

////@begin VirtualH89Frame event table entries
    EVT_MENU( ID_MENUITEM_FileExit, VirtualH89Frame::OnMENUITEMFileExitClick )
    EVT_MENU( ID_MENUITEM_HelpAbout, VirtualH89Frame::OnMENUITEMHelpAboutClick )
////@end VirtualH89Frame event table entries

END_EVENT_TABLE()

VirtualH89Frame *TheFrame = nullptr;

int VirtualH89main(int argc, char* argv[]);

GUIwxWidgets *TheGUIwxWidgets = nullptr;

std::vector<wxBitmap> FontBitmaps;

/*
 * VirtualH89Frame constructors
 */

VirtualH89Frame::VirtualH89Frame()
{
    Init();
}

VirtualH89Frame::VirtualH89Frame( wxWindow* parent, wxWindowID id, const wxString& caption, const wxPoint& pos, const wxSize& size, long style )
{
    Init();
    Create( parent, id, caption, pos, size, style );
}


/*
 * VirtualH89Frame creator
 */

bool VirtualH89Frame::Create( wxWindow* parent, wxWindowID id, const wxString& caption, const wxPoint& pos, const wxSize& size, long style )
{
////@begin VirtualH89Frame creation
    wxFrame::Create( parent, id, caption, pos, size, style );

    CreateControls();
    if (GetSizer())
    {
        GetSizer()->SetSizeHints(this);
    }
    Centre();
////@end VirtualH89Frame creation
    return true;
}


/*
 * VirtualH89Frame destructor
 */

VirtualH89Frame::~VirtualH89Frame()
{
////@begin VirtualH89Frame destruction
////@end VirtualH89Frame destruction
}


/*
 * Member initialisation
 */

void VirtualH89Frame::Init()
{
////@begin VirtualH89Frame member initialisation
    m_H19TextFrame = NULL;
////@end VirtualH89Frame member initialisation
}


/*
 * Control creation for VirtualH89Frame
 */

void VirtualH89Frame::CreateControls()
{    
////@begin VirtualH89Frame content construction
    VirtualH89Frame* itemFrame1 = this;

    wxMenuBar* menuBar = new wxMenuBar;
    wxMenu* itemMenu3 = new wxMenu;
    itemMenu3->Append(ID_MENUITEM_FileExit, _("E&xit"), wxEmptyString, wxITEM_NORMAL);
    menuBar->Append(itemMenu3, _("&File"));
    wxMenu* itemMenu5 = new wxMenu;
    itemMenu5->Append(ID_MENUITEM_ViewConfig, _("&Config"), wxEmptyString, wxITEM_NORMAL);
    itemMenu5->Enable(ID_MENUITEM_ViewConfig, false);
    menuBar->Append(itemMenu5, _("View"));
    wxMenu* itemMenu7 = new wxMenu;
    itemMenu7->Append(ID_MENUITEM_HelpAbout, _("About"), wxEmptyString, wxITEM_NORMAL);
    menuBar->Append(itemMenu7, _("Help"));
    itemFrame1->SetMenuBar(menuBar);

    wxBoxSizer* itemBoxSizer9 = new wxBoxSizer(wxVERTICAL);
    itemFrame1->SetSizer(itemBoxSizer9);

    m_H19TextFrame = new H19TextFrame( itemFrame1, ID_SCROLLEDWINDOW_H19Text, wxDefaultPosition, wxSize(640, 480), wxSUNKEN_BORDER );
    m_H19TextFrame->SetForegroundColour(wxColour(255, 255, 255));
    m_H19TextFrame->SetBackgroundColour(wxColour(0, 0, 0));
    itemBoxSizer9->Add(m_H19TextFrame, 1, wxGROW|wxALL, 2);

////@end VirtualH89Frame content construction

    // Create the VirtualH89/GUI interface engine for a wxWidgets engine
    TheGUIwxWidgets = new GUIwxWidgets();
    TheGUI = TheGUIwxWidgets;

    // Make the frame visible to the GUI interface engine
    TheFrame = this;

    // Load the fonts into bitmaps.
    wxImage CharImage(8, 20);
    auto Pixels = CharImage.GetData();

    std::bitset<8> CharBits;

    for (auto Char = 0u; Char < 256u; ++Char)
    {
      const unsigned char *Line = fontTableForward + (Char * 20u);
      for (auto y = 0u; y < 20u; ++y)
      {
        CharBits = Line[y];
        for (auto x = 0u; x < 8u; ++x)
        {
          auto Index = 3u * ((y * 8u) + x);
          auto Color = CharBits[7u - x] ? 255u : 0u;
          Pixels[Index++] = Color;  // Red
          Pixels[Index++] = Color;  // Green
          Pixels[Index++] = 0u;     // Blue
        }
      }
      FontBitmaps.push_back(wxBitmap(CharImage));
    }

    // Start VirtualH89 main program.
    VirtualH89main(wxApp::GetInstance()->argc, wxApp::GetInstance()->argv);

    return;
}


/*
 * Should we show tooltips?
 */

bool VirtualH89Frame::ShowToolTips()
{
    return true;
}

/*
 * Get bitmap resources
 */

wxBitmap VirtualH89Frame::GetBitmapResource( const wxString& name )
{
    // Bitmap retrieval
////@begin VirtualH89Frame bitmap retrieval
    wxUnusedVar(name);
    return wxNullBitmap;
////@end VirtualH89Frame bitmap retrieval
}

/*
 * Get icon resources
 */

wxIcon VirtualH89Frame::GetIconResource( const wxString& name )
{
    // Icon retrieval
////@begin VirtualH89Frame icon retrieval
    wxUnusedVar(name);
    return wxNullIcon;
////@end VirtualH89Frame icon retrieval
}


/*
 * wxEVT_COMMAND_MENU_SELECTED event handler for ID_MENUITEM_HelpAbout
 */

void VirtualH89Frame::OnMENUITEMHelpAboutClick( wxCommandEvent& event )
{
////@begin wxEVT_COMMAND_MENU_SELECTED event handler for ID_MENUITEM_HelpAbout in VirtualH89Frame.
    // Before editing this code, remove the block markers.
    event.Skip();
////@end wxEVT_COMMAND_MENU_SELECTED event handler for ID_MENUITEM_HelpAbout in VirtualH89Frame. 

  AboutVirtualH89 *AboutDialog = new AboutVirtualH89(this);
  AboutDialog->ShowModal();
  
  AboutDialog->Destroy();
  
  return;
}


/*
 * wxEVT_COMMAND_MENU_SELECTED event handler for ID_MENUITEM_FileExit
 */

void VirtualH89Frame::OnMENUITEMFileExitClick( wxCommandEvent& event )
{
////@begin wxEVT_COMMAND_MENU_SELECTED event handler for ID_MENUITEM_FileExit in VirtualH89Frame.
    // Before editing this code, remove the block markers.
    event.Skip();
////@end wxEVT_COMMAND_MENU_SELECTED event handler for ID_MENUITEM_FileExit in VirtualH89Frame. 

  Close();
  
  return;
}
#endif
