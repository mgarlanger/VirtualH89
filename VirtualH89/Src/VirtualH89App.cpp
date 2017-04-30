#if defined(__GUIwx__)
/////////////////////////////////////////////////////////////////////////////
// Name:        VirtualH89App.cpp
// Purpose:     
// Author:      
// Modified by: 
// Created:     Sat 22 Apr 2017 16:38:00 CDT
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

#include "VirtualH89App.h"

////@begin XPM images
////@end XPM images


/*
 * Application instance implementation
 */

////@begin implement app
IMPLEMENT_APP( VirtualH89App )
////@end implement app


/*
 * VirtualH89App type definition
 */

IMPLEMENT_CLASS( VirtualH89App, wxApp )


/*
 * VirtualH89App event table definition
 */

BEGIN_EVENT_TABLE( VirtualH89App, wxApp )

////@begin VirtualH89App event table entries
////@end VirtualH89App event table entries

END_EVENT_TABLE()


/*
 * Constructor for VirtualH89App
 */

VirtualH89App::VirtualH89App()
{
    Init();
}


/*
 * Member initialisation
 */

void VirtualH89App::Init()
{
////@begin VirtualH89App member initialisation
////@end VirtualH89App member initialisation
}

/*
 * Initialisation for VirtualH89App
 */

bool VirtualH89App::OnInit()
{    
////@begin VirtualH89App initialisation
	// Remove the comment markers above and below this block
	// to make permanent changes to the code.

#if wxUSE_XPM
	wxImage::AddHandler(new wxXPMHandler);
#endif
#if wxUSE_LIBPNG
	wxImage::AddHandler(new wxPNGHandler);
#endif
#if wxUSE_LIBJPEG
	wxImage::AddHandler(new wxJPEGHandler);
#endif
#if wxUSE_GIF
	wxImage::AddHandler(new wxGIFHandler);
#endif
	VirtualH89Frame* mainWindow = new VirtualH89Frame( NULL );
	mainWindow->Show(true);
////@end VirtualH89App initialisation

    return true;
}


/*
 * Cleanup for VirtualH89App
 */

int VirtualH89App::OnExit()
{    
////@begin VirtualH89App cleanup
	return wxApp::OnExit();
////@end VirtualH89App cleanup
}
#endif
