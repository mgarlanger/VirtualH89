#if defined(__GUIwx__)
/////////////////////////////////////////////////////////////////////////////
// Name:        aboutvirtualh89.cpp
// Purpose:     
// Author:      
// Modified by: 
// Created:     Sat 22 Apr 2017 17:10:14 CDT
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

#include "AboutVirtualH89.h"

////@begin XPM images
////@end XPM images


/*
 * AboutVirtualH89 type definition
 */

IMPLEMENT_DYNAMIC_CLASS( AboutVirtualH89, wxDialog )


/*
 * AboutVirtualH89 event table definition
 */

BEGIN_EVENT_TABLE( AboutVirtualH89, wxDialog )

////@begin AboutVirtualH89 event table entries
////@end AboutVirtualH89 event table entries

END_EVENT_TABLE()


/*
 * AboutVirtualH89 constructors
 */

AboutVirtualH89::AboutVirtualH89()
{
    Init();
}

AboutVirtualH89::AboutVirtualH89( wxWindow* parent, wxWindowID id, const wxString& caption, const wxPoint& pos, const wxSize& size, long style )
{
    Init();
    Create(parent, id, caption, pos, size, style);
}


/*
 * AboutVirtualH89 creator
 */

bool AboutVirtualH89::Create( wxWindow* parent, wxWindowID id, const wxString& caption, const wxPoint& pos, const wxSize& size, long style )
{
////@begin AboutVirtualH89 creation
    SetExtraStyle(wxWS_EX_VALIDATE_RECURSIVELY|wxWS_EX_BLOCK_EVENTS);
    wxDialog::Create( parent, id, caption, pos, size, style );

    CreateControls();
    if (GetSizer())
    {
        GetSizer()->SetSizeHints(this);
    }
    Centre();
////@end AboutVirtualH89 creation
    return true;
}


/*
 * AboutVirtualH89 destructor
 */

AboutVirtualH89::~AboutVirtualH89()
{
////@begin AboutVirtualH89 destruction
////@end AboutVirtualH89 destruction
}


/*
 * Member initialisation
 */

void AboutVirtualH89::Init()
{
////@begin AboutVirtualH89 member initialisation
////@end AboutVirtualH89 member initialisation
}


/*
 * Control creation for AboutVirtualH89
 */

void AboutVirtualH89::CreateControls()
{    
////@begin AboutVirtualH89 content construction
    AboutVirtualH89* itemDialog1 = this;

    wxBoxSizer* itemBoxSizer2 = new wxBoxSizer(wxVERTICAL);
    itemDialog1->SetSizer(itemBoxSizer2);

    wxTextCtrl* itemTextCtrl3 = new wxTextCtrl( itemDialog1, ID_TEXTCTRL_AboutText, _("Virtual H89\n\n #     # ### ####  ##### #   #   #   #       #   #  ###   ### \n #     #  #  #   #   #   #   #  # #  #       #   # #   # #   #\n  #   #   #  #   #   #   #   # #   # #       #   # #   # #   #\n  #   #   #  ####    #   #   # ##### #       #####  ###   ####\n   # #    #  #  #    #   #   # #   # #       #   # #   #     #\n   # #    #  #   #   #   #   # #   # #       #   # #   # #   #\n    #    ### #   #   #    ###  #   # #####   #   #  ###   ### \n\nPortions derived from Z80Pack Release 1.17- Copyright (C) 1987-2008 by Udo Munk\nVirtual H89 - Copyright (C) 2009-2016 by Mark Garlanger\nRelease 1.93\n"), wxDefaultPosition, wxSize(670, 230), wxTE_MULTILINE|wxTE_READONLY );
    itemTextCtrl3->SetFont(wxFont(10, wxFONTFAMILY_TELETYPE, wxFONTSTYLE_NORMAL, wxFONTWEIGHT_NORMAL, false, wxT("Courier")));
    itemBoxSizer2->Add(itemTextCtrl3, 1, wxGROW|wxALL, 2);

////@end AboutVirtualH89 content construction
}


/*
 * Should we show tooltips?
 */

bool AboutVirtualH89::ShowToolTips()
{
    return true;
}

/*
 * Get bitmap resources
 */

wxBitmap AboutVirtualH89::GetBitmapResource( const wxString& name )
{
    // Bitmap retrieval
////@begin AboutVirtualH89 bitmap retrieval
    wxUnusedVar(name);
    return wxNullBitmap;
////@end AboutVirtualH89 bitmap retrieval
}

/*
 * Get icon resources
 */

wxIcon AboutVirtualH89::GetIconResource( const wxString& name )
{
    // Icon retrieval
////@begin AboutVirtualH89 icon retrieval
    wxUnusedVar(name);
    return wxNullIcon;
////@end AboutVirtualH89 icon retrieval
}
#endif
