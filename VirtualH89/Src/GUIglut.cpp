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

#include "h19-font.h"

tKeyboardFunc GUIglut::GUIKeyboardFunc = nullptr;
tDisplayFunc GUIglut::GUIDisplayFunc;
tDisplayFunc GUIglut::GUITimerFunc;

unsigned int GUIglut::m_ms;

GUIglut::GUIglut()
{
  // TODO Auto-generated constructor stub

}

GUIglut::~GUIglut()
{
  // TODO Auto-generated destructor stub
}

void GUIglut::InitGUI(void)
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

  H19::GetH19()->fontOffset_m = glGenLists(0x101);

  for (i = 0; i < 0x100; i++)
  {
      glNewList(H19::GetH19()->fontOffset_m + i, GL_COMPILE);
      glBitmap(8, 20, 0.0, 0.0, 0.0, -20.0, &fontTable[i * 20]);
      glEndList();
  }

  // Special character to wrap around.
  glNewList(H19::GetH19()->fontOffset_m + 0x100, GL_COMPILE);
  glBitmap(8, 20, 0.0, 0.0, 8.0, 500.0, &fontTable[32 * 20]);
  glEndList();

  glutReshapeFunc(reshape);
  glutSpecialFunc(special);

  glutIgnoreKeyRepeat(1);

  return;
}

void GUIglut::SetTimerFunc(unsigned int ms, tTimerFunc TimerFunc)
{
  assert(TimerFunc);

  m_ms = ms;
  GUITimerFunc = TimerFunc;

  glutTimerFunc(ms, GLUTTimerFunc, 1);

  return;

}

void GUIglut::GLUTTimerFunc(int i)
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

void GUIglut::SetDisplayFunc(tDisplayFunc DisplayFunc)
{
  assert(DisplayFunc);

  GUIDisplayFunc = DisplayFunc;
  glutDisplayFunc(GLUTDisplayFunc);

  return;

}

void GUIglut::GLUTDisplayFunc(void)
{
  assert(GUIDisplayFunc);
  GUIDisplayFunc();

  return;
}

void GUIglut::SetKeyboardFunc(tKeyboardFunc KeyboardFunc)
{
  assert(KeyboardFunc);

  GUIKeyboardFunc = KeyboardFunc;
  glutKeyboardFunc(GLUTKeyboardFunc);

  return;

}

void GUIglut::GLUTKeyboardFunc(unsigned char Key, int x, int y)
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

