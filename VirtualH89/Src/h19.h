/// \file h19.h
///
/// \date Apr 12, 2009
/// \author Mark Garlanger
///

#ifndef H19_H_
#define H19_H_


#include "Console.h"
#include "ClockUser.h"

//
// OpenGL Headers
//
/// \cond
#ifdef __APPLE__
#include <OpenGL/gl.h>
#else
#include <GL/gl.h>
#endif
#include <queue>
/// \endcond



/// \brief Virtual %H19 %Terminal
///
/// Virtual Heathkit H19 Terminal that utilizes the GLUT framework to
/// render a pixel accurate emulation of the terminal.
///
class H19: public Console, public ClockUser // , public BaseThread
{
  public:
    H19(std::string sw401 = "10001100", std::string sw402 = "00000101");
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
    void notification(unsigned int cycleCount);
    virtual bool sendData(BYTE data);

  private:

    static void glDisplay();
    static void reshape(int w,
                        int h);
    static void timer(int i);
    static void keyboard(unsigned char key,
                         int           x,
                         int           y);
    static void special(int key,
                        int x,
                        int y);
    static H19*               h19; // for static callback funcs
    static const unsigned int screenRefresh_c = 1000 / 60;
    static unsigned int       screenRefresh_m;
    void initGl();

    void consoleLog(std::string message);

    // screen size
    static const unsigned int cols_c     = 80;
    static const unsigned int rows_c     = 25;
    static const unsigned int rowsMain_c = 24;

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
    InputMode        mode_m;
    bool             updated_m;
    BYTE             sw401_m;
    BYTE             sw402_m;

    std::queue<BYTE> charToSend;
    unsigned long    countdownToSend_m;
    unsigned long    characterDelay_m;

    // display modes
    bool             reverseVideo_m;
    bool             graphicMode_m;
    bool             insertMode_m;
    bool             line25_m;
    bool             holdScreen_m;
    bool             cursorOff_m;
    bool             cursorBlock_m;
    bool             wrapEOL_m;
    bool             autoLF_m;
    bool             autoCR_m;

    bool             keyboardEnabled_m;
    bool             keyClick_m;
    bool             keypadShifted_m;
    bool             altKeypadMode_m;

    bool             offline_m;

    GLuint           fontOffset_m;
    GLuint           screen_m[cols_c][rows_c + 1]; // extra row for GLUT wraparound.

    bool             curCursor_m;
    unsigned int     posX_m, posY_m;
    unsigned int     saveX_m, saveY_m;

    //
    // internal routines to handle control characters.
    //

    // character handling
    virtual void processBS();
    virtual void processTAB();

    inline virtual void scroll()
    {
        for (unsigned int y = 0; y < (rowsMain_c - 1); ++y)
        {
            for (unsigned int x = 0; x < cols_c; ++x)
            {
                screen_m[x][y] = screen_m[x][y + 1];
            }
        }

        eraseLine(rowsMain_c - 1);
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

    virtual void transmitLines(int start,
                               int end);
    virtual void transmitLine25();
    virtual void transmitPage();
    virtual void displayCharacter(unsigned int ch);

    inline bool onLine25()
    {
        return (posX_m == (rows_c - 1));
    };
    virtual void bell(void);

    const BYTE RefreshRate_c  = 0x80;
    const BYTE KeypadMode_c   = 0x40;
    const BYTE TerminalMode_c = 0x20;
    const BYTE AutoCR_c       = 0x10;
    const BYTE AutoLF_c       = 0x08;
    const BYTE WrapEOL_c      = 0x04;
    const BYTE KeyClick_c     = 0x02;
    const BYTE BlockCursor    = 0x01;

    void setSW401(BYTE sw401);
    void setSW402(BYTE sw402);

};

#endif // H19_H_
