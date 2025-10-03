/// \file main.cpp
///
/// \date Mar 7, 2009
/// \author Mark Garlanger
///
///
///
///
///

#include "main.h"


#include "H89.h"
#include "Console.h"
#include "h19.h"
#include "StdioConsole.h"
#include "StdioProxyConsole.h"
#include "logger.h"
#include "propertyutil.h"

#if defined(__GUIimgui__)
#include "GUIimgui.h"
#elif !defined(__GUIwx__)
#include "GUIglut.h"
#endif

/// \cond
#include <iostream>
#include <signal.h>
#include <stdlib.h>
#include <string>
#include <cstring>
#include <unistd.h>
/// \endcond


using namespace std;

const char* Z80_COPYRIGHT_c   = "Portions derived from Z80Pack Release 1.17"
                                "- Copyright (C) 1987-2008 by Udo Munk";

const char* RELEASE_VERSION_c = "1.93";
const char* H89_COPYRIGHT_c   = "Copyright (C) 2009-2016 by Mark Garlanger";

const char* usage_str         = " -q -g";

/// \todo - make H89 into a singleton.
H89         h89;
Console*    console     = nullptr;

FILE*       log_out     = 0;
FILE*       console_out = 0;

void
displayLogo()
{
    cout << "Virtual H89" << endl << endl;

    cout << " #     # ### ####  ##### #   #   #   #       #   #  ###   ### " << endl;
    cout << " #     #  #  #   #   #   #   #  # #  #       #   # #   # #   #" << endl;
    cout << "  #   #   #  #   #   #   #   # #   # #       #   # #   # #   #" << endl;
    cout << "  #   #   #  ####    #   #   # ##### #       #####  ###   ####" << endl;
    cout << "   # #    #  #  #    #   #   # #   # #       #   # #   #     #" << endl;
    cout << "   # #    #  #   #   #   #   # #   # #       #   # #   # #   #" << endl;
    cout << "    #    ### #   #   #    ###  #   # #####   #   #  ###   ### " << endl;
    cout << endl << Z80_COPYRIGHT_c << endl;
    cout << "Virtual H89 - " << H89_COPYRIGHT_c << endl << endl;
    cout << "Release " << RELEASE_VERSION_c << endl;
}


void
usage(char* pn)
{
    cout << "Virtual Heathkit H-89 All-in-One Computer Emulator" << endl;
    cout << "usage: " << pn << usage_str << endl << endl;
    cout << "Options:" << endl;
    cout << "  -g <gui>        Specify GUI to use (default: built-in H19 emulation)" << endl;
    cout << "  -h, --help      Show this help message" << endl;
    cout << "  -q              Quiet mode - don't display opening banner" << endl << endl;
}

static void*
cpuThreadFunc(void* v)
{
    sigset_t set;

    sigemptyset(&set);
    sigaddset(&set, SIGALRM);
    pthread_sigmask(SIG_UNBLOCK, &set, 0);
    H89* h89 = (H89*) v;

#if FIXME

    if (l_flag)
    {
        if (load_core())
        {
            return (1);
        }
    }

#endif

    BYTE cpu_error;

    cpu_error = h89->run();


#if FIXME

    if (s_flag)
    {
        save_core();
    }

#endif

    return (0);
}

// This is the getopt string for ALL consumers of argc/argv.
// Right now, it must be manually kept up to date.
//
//	option		owner
//	-g <gui>	main.cpp
//	-h		main.cpp (handled by early check, not getopt)
//	-l		StdioProxyConsole.cpp
//	-q		main.cpp
//
const char* getopts = "g:lq";

#if defined(__GUIwx__)
int
VirtualH89main(int   argc,
     char* argv[])
#else
int
main(int   argc,
     char* argv[])
#endif
{
    // Check for help flag FIRST, before any initialization
    for (int i = 1; i < argc; i++) {
        if (strcmp(argv[i], "-h") == 0 || strcmp(argv[i], "--help") == 0) {
            usage(argv[0]);
            _exit(0);  // Fast exit without static destructors (prevents segfault)
        }
    }

    int          c;
    extern char* optarg;
    string       gui("H19");
    int          quiet = 0;
    setDebugLevel();

#if defined(__GUIimgui__)
    // Start ImGui GUI engine
    TheGUI = new GUIimgui();
#elif !defined(__GUIwx__)
    // Start GLUT GUI engine
    TheGUI = new GUIglut();
#endif

    while ((c = getopt(argc, argv, getopts)) != EOF)
    {
        switch (c)
        {
            case 'q':
                quiet = 1;
                break;

            case 'g':
                gui = optarg;
                break;
        }
    }

    if ((log_out = fopen("op.out", "w")) == 0 && !quiet)
    {
        cerr << endl << "Unable to open op.out" << endl;
    }

    if ((console_out = fopen("console.out", "w")) == 0 && !quiet)
    {
        cerr << endl << "Unable to open console.out" << endl;
    }
    else if (!quiet)
    {
        cout << "Successfully opened console.out" << endl;
    }

    if (!quiet)
    {
        displayLogo();
        cout << "CPU speed is 2.048 MHz" << endl << endl;
    }

    sigset_t set;

    sigemptyset(&set);
    sigaddset(&set, SIGINT);
    sigaddset(&set, SIGALRM);
    pthread_sigmask(SIG_BLOCK, &set, 0);

    // TODO: allow specification of config file via cmdline args.
    string                     cfg;
    char*                      env = getenv("V89_CONFIG");
    PropertyUtil::PropertyMapT props;
    string                     sw401;
    string                     sw402;

    // \todo - if file not found, default to something sane (h17 + 64k)
    if (env)
    {
        // If file-not-found, we still might create it later...
        cfg = env;

        try
        {
            PropertyUtil::read(cfg.c_str(), props);
        }

        catch (exception& e)
        {
        }
    }
    else
    {
        cfg  = getenv("HOME");
        cfg += "/.v89rc";

        try
        {
            PropertyUtil::read(cfg.c_str(), props);
        }

        catch (exception& e)
        {
        }
    }

    sw401 = props["sw401"];
    sw402 = props["sw402"];

    if (gui.compare("stdio") == 0)
    {
        console = new StdioConsole(argc, argv);
    }
    else if (gui.compare("proxy") == 0)
    {
        console = new StdioProxyConsole(argc, argv);
    }
    else
    {
        console = new H19(sw401.c_str(), sw402.c_str());
    }

    h89.buildSystem(console, props);

    pthread_t cpuThread;
    pthread_create(&cpuThread, nullptr, cpuThreadFunc, &h89);
    h89.init();

    console->run();

    // TODO: call destructors...

    // Leave open so destructors can log messages.
    // exit() always closes files anyway.
    // fclose(log_out);
    // fclose(console_out);
    return (0);
}
