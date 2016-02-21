/// \file main.cpp
///
/// \date Mar 7, 2009
/// \author Mark Garlanger
///
///
/// \todo
///
///
/// 3. Get interrupt mode 0 working.
///    - for any type of instruction.
///
/// 6. Fix intermittent issue with 'pure virtual class' when exiting program.
///

#include "config.h"
#include <iostream>
#include <signal.h>
#include <stdlib.h>
#include <string>
#include <unistd.h>

#include "main.h"
#include "H89.h"
#include "Console.h"
#include "h19.h"
#include "StdioConsole.h"
#include "StdioProxyConsole.h"
#include "logger.h"

using namespace std;

#define Z80COPYRIGHT "Copyright (C) 1987-2008 by Udo Munk"
#define RELEASE "1.17"
#define H89COPYRIGHT "Copyright (C) 2009-2016 by Mark Garlanger"

const char *usage_str = " -b -s -l";

/// \todo - make H89 into a singleton.
H89 h89;
Console *console = NULL;

FILE *log_out = 0;
FILE *console_out = 0;

void usage(char *pn)
{
    cerr << "usage: " << pn << usage_str << endl;
    cerr << "\ts = save core and cpu" << endl;
    cerr << "\tl = load core and cpu" << endl;
    cerr << "\tb = display opening banner" << endl;
    exit(1);
}

#if USE_PTHREAD
static void *cpuThreadFunc(void *v)
{
    sigset_t set;

    sigemptyset(&set);
    sigaddset(&set, SIGALRM);
    pthread_sigmask(SIG_UNBLOCK, &set, 0);
    H89 *h89 = (H89 *)v;

#if FIXME

    if (l_flag)
    {
        if (load_core())
        {
            return (1);
        }
    }

#else
    //h89->clearMemory(0);
    //h89->reset();
#endif
#if 0
    powerUp();
#else
    BYTE cpu_error;

    // load boot code into memory
    cpu_error = h89->run();
#endif

#if FIXME

    if (s_flag)
    {
        save_core();
    }

#endif

    return (0);
}
#endif

/// \todo - use argv[0] to determine configuration... ie H88 vs. H89 vs. Z90.
int main(int argc, char *argv[])
{
    int c;
    extern char *optarg;
    std::string gui("H19");
    int quiet = 0;
    setDebugLevel();

    while ((c = getopt(argc, argv, "qg:")) != EOF)
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
        cout << "Virtual H89" << endl << endl;

        cout << " #       #  #####  #####   #######  #     #    ###    #           #     #   #####    ##### " << endl;
        cout << "  #     #     #    #    #     #     #     #   #   #   #           #     #  #     #  #     #" << endl;
        cout << "  #     #     #    #    #     #     #     #  #     #  #           #     #  #     #  #     #" << endl;
        cout << "   #   #      #    #####      #     #     #  #######  #           #######   #####    ######" << endl;
        cout << "   #   #      #    #   #      #     #     #  #     #  #           #     #  #     #        #" << endl;
        cout << "    # #       #    #    #     #     #     #  #     #  #           #     #  #     #  #     #" << endl;
        cout << "     #      #####  #    #     #      #####   #     #  ######      #     #   #####    ##### " << endl;
        cout << endl << "Portions derived from Z80Pack Release " << RELEASE << " - "
             << Z80COPYRIGHT << endl;
        cout << "Virtual H89 - " << H89COPYRIGHT << endl << endl;
        cout << "CPU speed is 2.048 MHz" << endl << endl;
    }

#if USE_PTHREAD

    sigset_t set;

    sigemptyset(&set);
    sigaddset(&set, SIGINT);
    sigaddset(&set, SIGALRM);
    pthread_sigmask(SIG_BLOCK, &set, 0);

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
        console = new H19();
    }

    h89.buildSystem(console);

    pthread_t thread;
    pthread_create(&thread, NULL, cpuThreadFunc, &h89);
    h89.init();

    console->run();

    // TODO: call destructors...
#endif  // USE_PTHREAD

    // Leave open so destructors can log messages.
    // exit() always closes files anyway.
    //fclose(log_out);
    //fclose(console_out);
    return (0);
}
