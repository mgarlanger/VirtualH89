/// \file h19.h
///
/// \date Apr 12, 2009
/// \author Mark Garlanger
///

#ifndef H19_H_
#define H19_H_

//
// OpenGL Headers
//
#ifdef __APPLE__
#include <OpenGL/gl.h>
#else
#include <GL/gl.h>
#endif

#include <assert.h>

#include "config.h"
#include "Console.h"

/// \brief Virtual %H19 %Terminal
///
/// Virtual Heathkit H19 Terminal that utilizes the GLUT framework to
/// render a pixel accurate emulation of the terminal.
///
class H19 : public Console //, public BaseThread
{
  public:
    H19();
    virtual ~H19();

    virtual void init();
    virtual void reset();

    virtual void display();
    virtual void processCharacter(char ch);

    virtual void keypress(char ch);

    virtual void receiveData(BYTE);

    virtual bool checkUpdated();

    virtual unsigned int getBaudRate();

    virtual void run();

  private:
    static void glDisplay();
    static void reshape(int w, int h);
    static void timer(int i);
    static void keyboard(unsigned char key, int x, int y);
    static void special(int key, int x, int y);
    static H19 *h19; // for static callback funcs
    static const unsigned int screenRefresh_c = 1000 / 60;
    static unsigned int screenRefresh;
    void initGl();

    // screen size
    static const unsigned int cols = 80;
    static const unsigned int rows = 25;
    static const unsigned int rowsMain = 24;

    // state variables
    enum InputMode
    {
        Normal,
        Escape,
        DCA_1,
        DCA_2,
        SetMode,
        ResetMode
    };
    InputMode mode;
    bool updated;

    // display modes
    bool reverseVideo;
    bool graphicMode;
    bool insertMode;
    bool line25;
    bool holdScreen;
    bool cursorOff;
    bool cursorBlock;
    bool wrapEOL;
    bool autoLF;
    bool autoCR;

    bool keyboardEnabled;
    bool keyClick;
    bool keypadShifted;
    bool altKeypadMode;

    bool offline;

    GLuint fontOffset;
    GLuint screen[cols][rows + 1]; // extra row for GLUT wraparound.

    bool curCursor;
    unsigned int  PosX, PosY;
    unsigned int  SaveX, SaveY;

    //
    // internal routines to handle control characters.
    //

    // character handling
    virtual void processBS();
    virtual void processTAB();

    inline virtual void scroll()
    {
        for (unsigned int y = 0; y < (rowsMain - 1); ++y)
        {
            for (unsigned int x = 0; x < cols; ++x)
            {
                screen[x][y] = screen[x][y + 1];
            }
        }

        eraseLine(rowsMain - 1);
    };

    //
    // internal routines to handle ESC sequences.
    //

    // Cursor
    virtual void cursorHome();
    virtual void cursorForward();
    // virtual void cursorBackward(); // unneeded same as processBS.
    virtual void cursorDown();
    virtual void cursorUp();
    virtual void reverseIndex();
    virtual void cursorPositionReport();
    virtual void saveCursorPosition();
    virtual void restoreCursorPosition();

    // Erasing and Editing
    virtual void clearDisplay();
    virtual void eraseBOD();
    virtual void eraseEOP();
    virtual void eraseEL();
    virtual void eraseBOL();
    virtual void eraseEOL();
    virtual void insertLine();
    virtual void deleteLine();
    virtual void deleteChar();
    virtual void eraseLine(unsigned int line);

    //
    virtual void processLF();
    virtual void processCR();
    virtual void processEnableLine25();

    virtual void transmitLines(int start, int end);
    virtual void transmitLine25();
    virtual void transmitPage();
    virtual void displayCharacter(unsigned int ch);

    inline bool onLine25()
    {
        return (PosX == (rows - 1));
    };
    virtual void bell(void);
};

#endif // H19_H_
