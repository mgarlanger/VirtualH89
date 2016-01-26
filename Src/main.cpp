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

#if OGL
#ifdef __APPLE__
#include <OpenGL/gl.h>
#include <OpenGL/glu.h>
#include <GLUT/glut.h>
#else
#include <GL/gl.h>
#include <GL/glut.h>
#endif
#else
#ifdef __APPLE__
#include <OpenGL/gl.h>
#else
#include <GL/gl.h>
#endif
#include <wx/wx.h>

#endif

#include "main.h"
#include "H89.h"
#include "logger.h"

const unsigned int screenRefresh_c = 1000/60;
unsigned int screenRefresh;

using namespace std;

#define Z80COPYRIGHT "Copyright (C) 1987-2008 by Udo Munk"
#define RELEASE "1.17"
#define H89COPYRIGHT "Copyright (C) 2009-2016 by Mark Garlanger"

const char *usage_str = " -b -s -l";

/// \todo - make H89 into a singleton.
H89 h89;

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
static void* cpuThreadFunc(void* v)
{
    sigset_t set;

    sigemptyset(&set);
    sigaddset(&set, SIGALRM);
    pthread_sigmask(SIG_UNBLOCK, &set, 0);

#if FIXME
    if (l_flag)
    {
        if (load_core())
        {
            return(1);
        }
    }
#else
    h89.clearMemory(0);
    h89.reset();
#endif
#if 0
    powerUp();
#else
    BYTE cpu_error;

    // load boot code into memory
    cpu_error = h89.run();
#endif

#if FIXME
    if (s_flag)
    {
        save_core();
    }
#endif

    return(0);
}
#endif

#if OGL

void display(void)
{
   h89.display();
}

void reshape(int w, int h)
{
    glViewport(0, 0, (GLsizei) w, (GLsizei) h);

    glMatrixMode(GL_PROJECTION);
    glLoadIdentity();
    glOrtho(0.0, w, 0.0, h, -1.0, 1.0);

    glMatrixMode(GL_MODELVIEW);
    glClearColor(0.0f, 0.0f, 0.0f, 0.9f);
    glClear(GL_COLOR_BUFFER_BIT);
}

void timer(int i)
{
    static int count = 0;

    if ((++count % 60) == 0)
    {
        fflush(console_out);
    }
    // Tell glut to redisplay the scene:
    if (h89.checkUpdated())
    {
        glutPostRedisplay();
    }

    // Need to call this method again after the desired amount of time has passed:
    glutTimerFunc(screenRefresh, timer, i);
}

void keyboard(unsigned char key, int x, int y)
{
    h89.keypress(key);
}

void special(int key, int x, int y)
{
    switch(key)
    {
        case GLUT_KEY_UP:
            h89.keypress('8');
            break;
        case GLUT_KEY_DOWN:
            h89.keypress('2');
            break;
        case GLUT_KEY_LEFT:
            h89.keypress('4');
            break;
        case GLUT_KEY_RIGHT:
            h89.keypress('6');
            break;
        default:
            break;
    }
}
#endif

/// \todo - use argv[0] to determine configuration... ie H88 vs. H89 vs. Z90.
int main(int argc, char *argv[])
{
    setDebugLevel();

    screenRefresh = screenRefresh_c;

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


#if USE_PTHREAD

    sigset_t set;

    sigemptyset(&set);
    sigaddset(&set, SIGINT);
    sigaddset(&set, SIGALRM);
    pthread_sigmask(SIG_BLOCK, &set, 0);

    if ( (log_out = fopen("op.out","w")) == 0)
    {
        cerr << endl << "Unable to open op.out" << endl;
    }

    if ( (console_out = fopen("console.out","w")) == 0)
    {
        cerr << endl << "Unable to open console.out" << endl;
    }
    else
    {
        cout << "Successfully opened console.out" << endl;
    }

    pthread_t thread;
    pthread_create(&thread, NULL, &cpuThreadFunc, NULL);

#if OGL

    glutInit(&argc, argv);
    glutInitDisplayMode(GLUT_DOUBLE | GLUT_RGBA);
    glutInitWindowSize(640, 500);
    glutInitWindowPosition (500, 100);
    glutCreateWindow((char *) "Virtual Heathkit H-89 All-in-One Computer");

    glClearColor(0.0f, 0.0f, 0.0f, 0.9f);
    //glutInitDisplayMode(GLUT_DOUBLE | GLUT_RGBA);
    glBlendFunc(GL_ONE, GL_ONE);
    //glBlendEquation(GL_FUNC_ADD);
    glBlendColor(0.5, 0.5, 0.5, 0.9);
    //glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);
    glEnable(GL_BLEND);

    glShadeModel (GL_FLAT);

    h89.init();
    glutReshapeFunc(reshape);
    glutKeyboardFunc(keyboard);
    glutSpecialFunc(special);
    glutDisplayFunc(display);
    glutTimerFunc(screenRefresh, timer, 1);
    glutIgnoreKeyRepeat(1);

    glutMainLoop();
    pthread_exit((void*) 0);

#endif  // OGL
#endif  // USE_PTHREAD

    fclose(log_out);
    fclose(console_out);
    return(0);
}
