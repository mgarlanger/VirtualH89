#if defined(__GUIglut__)
///
/// \name GUIglut.h
///
/// A GUI implementation based on glut.
///
/// \date Apr 20, 2017
/// \author David Troendle and Mark Garlanger
///

#include <cassert>
#include "GUIglut.h"

tKeyboardFunc GUIglut::GUIKeyboardFunc = nullptr;
tDisplayFunc  GUIglut::GUIDisplayFunc;
tDisplayFunc  GUIglut::GUITimerFunc;

unsigned int  GUIglut::m_ms;

// GLUT routine used to redisplay the screen when needed.

#define max(a, b)    ((a) > (b) ? (a) : (b))
#define min(a, b)    ((a) < (b) ? (a) : (b))

GUIglut::GUIglut()
{
    // Set inverted character generator for GLUT.
    fontTable = (unsigned char*) fontTableInverted;

    return;
}

GUIglut::~GUIglut()
{
    // TODO Auto-generated destructor stub
}

void
GUIglut::GUIDisplay(void)
{
    GLfloat color[3] = {1.0, 1.0, 0.0}; // amber
    // GLfloat color[3] = {0.0, 1.0, 0.0}; // green
    // GLfloat color[3] = { 1.0, 1.0, 1.0 };  // white
    // GLfloat color[3] = { 0.5, 0.0, 1.0 };  // purple
    // GLfloat color[3] = { 0.0, 0.8, 0.0 };
    // GLfloat color[3] = { 0.9, 0.9, 0.0 };  // amber

    glClear(GL_COLOR_BUFFER_BIT);
    glColor3fv(color);

    glRasterPos2i(20, 24 * 20 + 20);

    glPushAttrib(GL_LIST_BIT);
    glListBase(fontOffset_m);
    glCallLists(26 * H19::GetH19()->cols_c, GL_UNSIGNED_INT, (GLuint*) H19::GetH19()->screen_m);
    glPopAttrib();
    glEnable(GL_COLOR_LOGIC_OP);

    if ((!H19::GetH19()->cursorOff_m) && (H19::GetH19()->curCursor_m))
    {

        glRasterPos2i(20 + min(H19::GetH19()->posX_m, 79) * 8,
                      (24 - H19::GetH19()->posY_m) * 20 + 20);

        glPushAttrib(GL_LIST_BIT);
        //  glEnable(GL_COLOR_LOGIC_OP);
        glLogicOp(GL_COPY);
        glListBase(fontOffset_m);
        GLuint cursor = (H19::GetH19()->cursorBlock_m) ? (128 + 32) : 27;
        glCallLists(1, GL_UNSIGNED_INT, &cursor);
        glPopAttrib();
    }

    glLogicOp(GL_COPY);
    glutSwapBuffers();

    return;
}

void
GUIglut::InitGUI(void)
{
    assert(H19::GetH19());

    int   dummy_argc = 1;
    char* dummy_argv = (char*) "dummy";

    glutInit(&dummy_argc, &dummy_argv);
    glutInitDisplayMode(GLUT_DOUBLE | GLUT_RGBA);
    glutInitWindowSize(640 + 40, 500 + 40);
    glutInitWindowPosition(500, 100);
    glutCreateWindow((char*) "Virtual Heathkit H-89 All-in-One Computer");

    glClearColor(0.0f, 0.0f, 0.0f, 0.9f);
    // glutInitDisplayMode(GLUT_DOUBLE | GLUT_RGBA);
    glBlendFunc(GL_ONE, GL_ONE);
    // glBlendEquation(GL_FUNC_ADD);
    glBlendColor(0.5, 0.5, 0.5, 0.9);
    // glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);
    glEnable(GL_BLEND);

    glShadeModel(GL_FLAT);


    GLuint i;
    glPixelStorei(GL_UNPACK_ALIGNMENT, 1);

    fontOffset_m = glGenLists(0x101);

    for (i = 0; i < 0x100; i++)
    {
        glNewList(fontOffset_m + i, GL_COMPILE);
        glBitmap(8, 20, 0.0, 0.0, 0.0, -20.0, &fontTable[i * 20]);
        glEndList();
    }

    // Special character to wrap around.
    glNewList(fontOffset_m + 0x100, GL_COMPILE);
    glBitmap(8, 20, 0.0, 0.0, 8.0, 500.0, &fontTable[32 * 20]);
    glEndList();

    glutReshapeFunc(reshape);
    glutSpecialFunc(special);

    glutIgnoreKeyRepeat(1);

    return;
}

void
GUIglut::StartGUI(void)
{
    glutMainLoop();

    return;
}

void
GUIglut::SetTimerFunc(unsigned int ms, tTimerFunc TimerFunc)
{
    assert(TimerFunc);

    m_ms         = ms;
    GUITimerFunc = TimerFunc;

    glutTimerFunc(ms, GLUTTimerFunc, 1);

    return;

}

void
GUIglut::GLUTTimerFunc(int i)
{
    assert(GUITimerFunc);
    GUITimerFunc();


    // Tell glut to redisplay the scene:
    if (H19::GetH19()->checkUpdated())
    {
        glutPostRedisplay();
    }

    glutTimerFunc(m_ms, GLUTTimerFunc, i);

    return;
}

void
GUIglut::SetDisplayFunc(tDisplayFunc DisplayFunc)
{
    assert(DisplayFunc);

    GUIDisplayFunc = DisplayFunc;
    glutDisplayFunc(GLUTDisplayFunc);

    return;

}

void
GUIglut::GLUTDisplayFunc(void)
{
    assert(GUIDisplayFunc);
    GUIDisplayFunc();

    return;
}

void
GUIglut::SetKeyboardFunc(tKeyboardFunc KeyboardFunc)
{
    assert(KeyboardFunc);

    GUIKeyboardFunc = KeyboardFunc;
    glutKeyboardFunc(GLUTKeyboardFunc);

    return;

}

void
GUIglut::GLUTKeyboardFunc(unsigned char Key, int x, int y)
{
    assert(GUIKeyboardFunc);
    GUIKeyboardFunc(Key);

    return;
}


void
GUIglut::reshape(int w,
                 int h)
{
    glViewport(0, 0, (GLsizei) w, (GLsizei) h);

    glMatrixMode(GL_PROJECTION);
    glLoadIdentity();
    glOrtho(0.0, w, 0.0, h, -1.0, 1.0);

    glMatrixMode(GL_MODELVIEW);
    glClearColor(0.0f, 0.0f, 0.0f, 0.9f);
    glClear(GL_COLOR_BUFFER_BIT);
}


void
GUIglut::special(int key,
                 int x,
                 int y)
{
    assert(GUIKeyboardFunc);

    // NOTE: GLUT has already differentiated exact keystrokes
    // based on modern keyboard standards. Here we just encode
    // the modern key codes into something convenient to use
    // in the H19 class.
    switch (key)
    {
        case GLUT_KEY_F1:
            GUIKeyboardFunc('S' | 0x80);
            break;

        case GLUT_KEY_F2:
            GUIKeyboardFunc('T' | 0x80);
            break;

        case GLUT_KEY_F3:
            GUIKeyboardFunc('U' | 0x80);
            break;

        case GLUT_KEY_F4:
            GUIKeyboardFunc('V' | 0x80);
            break;

        case GLUT_KEY_F5:
            GUIKeyboardFunc('W' | 0x80);
            break;

        case GLUT_KEY_F6:
            GUIKeyboardFunc('P' | 0x80);
            break;

        case GLUT_KEY_F7:
            GUIKeyboardFunc('Q' | 0x80);
            break;

        case GLUT_KEY_F8:
            GUIKeyboardFunc('R' | 0x80);
            break;

        case GLUT_KEY_HOME:
            GUIKeyboardFunc('H' | 0x80);
            break;

        case GLUT_KEY_UP:
            GUIKeyboardFunc('A' | 0x80);
            break;

        case GLUT_KEY_DOWN:
            GUIKeyboardFunc('B' | 0x80);
            break;

        case GLUT_KEY_LEFT:
            GUIKeyboardFunc('D' | 0x80);
            break;

        case GLUT_KEY_RIGHT:
            GUIKeyboardFunc('C' | 0x80);
            break;

        default:
            break;
    }

    return;
}
#endif
