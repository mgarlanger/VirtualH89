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

#include <iostream>
#include <signal.h>
#include <stdlib.h>
#include <string>
#include <unistd.h>

#include "H89.h"
#include "Console.h"
#include "h19.h"
#include "StdioConsole.h"
#include "StdioProxyConsole.h"
#include "logger.h"
#include "propertyutil.h"


using namespace std;

const char* Z80_COPYRIGHT_c   =
    "Portions derived from Z80Pack Release 1.17 - Copyright (C) 1987-2008 by Udo Munk";

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
    cerr << "usage: " << pn << usage_str << endl;
    // cerr << "\ts = save core and cpu" << endl;
    // cerr << "\tl = load core and cpu" << endl;
    cerr << "\tg = specify gui to use, default is built-in H19 emulation" << endl;
    cerr << "\tq = quiet - don't display opening banner" << endl;
    exit(1);
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
//	-l		StdioProxyConsole.cpp
//	-q		main.cpp
//
const char* getopts = "g:lq";

int
main(int   argc,
     char* argv[])
{
    int          c;
    extern char* optarg;
    std::string  gui("H19");
    int          quiet = 0;
    setDebugLevel();

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
    std::string                cfg;
    char*                      env = getenv("V89_CONFIG");
    PropertyUtil::PropertyMapT props;
    std::string                sw401;
    std::string                sw402;

    // \todo - if file not found, default to something sane (h17 + 64k)
    if (env)
    {
        // If file-not-found, we still might create it later...
        cfg = env;

        try
        {
            PropertyUtil::read(cfg.c_str(), props);
        }

        catch (std::exception& e)
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

        catch (std::exception& e)
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
