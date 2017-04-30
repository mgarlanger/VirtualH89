#if defined(__GUIwx__)
/////////////////////////////////////////////////////////////////////////////
// Name:        VirtualH89App.h
// Purpose:     
// Author:      
// Modified by: 
// Created:     Sat 22 Apr 2017 16:38:00 CDT
// RCS-ID:      
// Copyright:   
// Licence:     
/////////////////////////////////////////////////////////////////////////////

#ifndef _VIRTUALH89APP_H_
#define _VIRTUALH89APP_H_


/*!
 * Includes
 */

////@begin includes
#include "wx/image.h"
#include "VirtualH89Frame.h"
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
////@end control identifiers

/*!
 * VirtualH89App class declaration
 */

class VirtualH89App: public wxApp
{    
    DECLARE_CLASS( VirtualH89App )
    DECLARE_EVENT_TABLE()

public:
    /// Constructor
    VirtualH89App();

    void Init();

    /// Initialises the application
    virtual bool OnInit();

    /// Called on exit
    virtual int OnExit();

////@begin VirtualH89App event handler declarations

////@end VirtualH89App event handler declarations

////@begin VirtualH89App member function declarations

////@end VirtualH89App member function declarations

////@begin VirtualH89App member variables
////@end VirtualH89App member variables
};

/*!
 * Application instance declaration 
 */

////@begin declare app
DECLARE_APP(VirtualH89App)
////@end declare app

#endif
    // _VIRTUALH89APP_H_
#endif
