/// \file h19.cpp
///
/// \date Apr 12, 2009
/// \author Mark Garlanger
///

#include "h19.h"

#include <iostream>
#include <signal.h>
#include <stdlib.h>
#include <pthread.h>
#include <unistd.h>
#include <assert.h>

#ifdef __APPLE__
#include <OpenGL/glu.h>
#include <GLUT/glut.h>
#else
#include <GL/glut.h>
#endif

#include "h19-font.h"
#include "logger.h"


#include "ascii.h"

static  pthread_mutex_t h19_mutex;

H19 *H19::h19;
unsigned int H19::screenRefresh = screenRefresh_c;

H19::H19(): Console(0, NULL),
    updated(true),
    offline(false),
    curCursor(false)
{
    h19 = this;
    pthread_mutex_init(&h19_mutex, NULL);
    reset();
}

H19::~H19()
{
    pthread_mutex_destroy(&h19_mutex);
}


bool H19::checkUpdated()
{
    pthread_mutex_lock(&h19_mutex);
    static unsigned int count = 0;

    if (!cursorOff)
    {
        if ((++count % 20) == 0)
        {
            curCursor = !curCursor;
            updated = true;
        }
    }

    bool tmp = updated;
    updated = false;
    pthread_mutex_unlock(&h19_mutex);
    return tmp;
}

void H19::init()
{
}

void H19::initGl()
{
    GLuint i;
    glPixelStorei(GL_UNPACK_ALIGNMENT, 1);

    fontOffset = glGenLists(0x101);

    for (i = 0; i < 0x100; i++)
    {
        glNewList(fontOffset + i, GL_COMPILE);
        glBitmap(8, 20, 0.0, 0.0, 0.0, -20.0, &fontTable[i * 20]);
        glEndList();
    }

    // Special character to wrap around.
    glNewList(fontOffset + 0x100, GL_COMPILE);
    glBitmap(8, 20, 0.0, 0.0, 8.0, 500.0, &fontTable[32 * 20]);
    glEndList();
}

#define CURSOR 1

// GLUT routine used to redisplay the screen when needed.

#define max(a,b)    ((a) > (b) ? (a) : (b))
#define min(a,b)    ((a) < (b) ? (a) : (b))

void H19::display()
{
    pthread_mutex_lock(&h19_mutex);
    //GLfloat color[3] = { 1.0, 1.0, 0.0 };  // amber
    GLfloat color[3] = { 0.0, 1.0, 0.0 };  // green
    //GLfloat color[3] = { 1.0, 1.0, 1.0 };  // white
    //GLfloat color[3] = { 0.5, 0.0, 1.0 };  // purple
    //GLfloat color[3] = { 0.0, 0.8, 0.0 };
    //GLfloat color[3] = { 0.9, 0.9, 0.0 };  // amber

    glClear(GL_COLOR_BUFFER_BIT);
    glColor3fv(color);

    glRasterPos2i(0, 24 * 20);

    glPushAttrib(GL_LIST_BIT);
    glListBase(h19->fontOffset);
    glCallLists(26 * cols, GL_UNSIGNED_INT, (GLuint *) screen);
    glPopAttrib();
    glEnable(GL_COLOR_LOGIC_OP);

#if CURSOR

    if ((!cursorOff) && (curCursor))
    {
#if 0
        glRasterPos2i(PosX * 8, (24 - PosY) * 20);
#else

        glRasterPos2i(min(PosX, 79) * 8, (24 - PosY) * 20);

#endif
        glPushAttrib(GL_LIST_BIT);
        //  glEnable(GL_COLOR_LOGIC_OP);
        glLogicOp(GL_COPY);
        glListBase(fontOffset);
        GLuint cursor = (cursorBlock) ? (128 + 32) : 27;
        glCallLists(1, GL_UNSIGNED_INT, &cursor);
        glPopAttrib();
    }

#endif

    glLogicOp(GL_COPY);
    glutSwapBuffers();
    pthread_mutex_unlock(&h19_mutex);
}

void H19::receiveData(BYTE ch)
{
    pthread_mutex_lock(&h19_mutex);
    processCharacter(ch);
    pthread_mutex_unlock(&h19_mutex);
}

void H19::keypress(char ch)
{
    pthread_mutex_lock(&h19_mutex);
    /// \todo - Send the key to the serial port unless offline.

    /// \todo fix this
    if (offline)
    {
        processCharacter(ch);
    }

    else
    {
        if ((ch & 0x80) != 0)
        {
            // TODO: modify keycode based on current terminal mode,
            // e.g. convert to ZDS or ANSI codes.
            // Note difference from H19 keyboard: a modern keyboard
            // has separate cursor keys that are always active,
            // so it is as if the user pressed SHIFT to get the code.
            sendData(ascii::ESC);
            usleep(2000);
            sendData(ch & 0x7f);
        }

        else
        {
            sendData(ch);
        }
    }

    pthread_mutex_unlock(&h19_mutex);
}

void H19::reset()
{
    /// \todo - make sure these modes are the defaults.
    /// \todo - some of these are affected by dipswitches - implement.
    mode            = Normal;
    reverseVideo    = false;
    graphicMode     = false;
    insertMode      = false;
    line25          = false;
    keyClick        = false;
    holdScreen      = false;
    cursorOff       = false;
    cursorBlock     = false;
    keypadShifted   = false;
    altKeypadMode   = false;
    autoLF          = false;
    autoCR          = false;
    keyboardEnabled = false;
    wrapEOL         = true;

    PosX  = PosY   = 0;
    SaveX = SaveY  = 0;

    for (unsigned int y = 0; y < rows; ++y)
    {
        eraseLine(y);
    }

    // specify the special symbol to end the line.
    for (unsigned int x = 0; x < cols; ++x)
    {
        screen[x][rows] = 0x100;
    }
}


void H19::processCharacter(char ch)
{
    // mask off the high bit just in case, the real H19 would not have it set.
    ch &= 0x7f;

    if (mode == Normal)
    {
        switch (ch)
        {
        case ascii::NUL:
        case ascii::SOH:
        case ascii::STX:
        case ascii::ETX:
        case ascii::EOT:
        case ascii::ENQ:
        case ascii::ACK:
        case ascii::VT:
        case ascii::FF:
        case ascii::SO:
        case ascii::SI:
        case ascii::DLE:
        case ascii::DC1:
        case ascii::DC2:
        case ascii::DC3:
        case ascii::DC4:
        case ascii::NAK:
        case ascii::SYN:
        case ascii::ETB:
        case ascii::EM:
        case ascii::SUB:
        case ascii::FS:
        case ascii::GS:
        case ascii::RS:
        case ascii::US:
        case ascii::DEL:
            // From manual, these characters are not processed by the terminal
            break;

        case ascii::BEL:   // Rings the bell.
            /// \todo - implement ringing bell.
#if CONSOLE_LOG
            fprintf(console_out, "<BEL>");
#endif

            break;

        case ascii::BS:    // Backspace
#if CONSOLE_LOG
            fprintf(console_out, "<BS>");
#endif
            processBS();
            break;

        case ascii::HT:    // Horizontal Tab
#if CONSOLE_LOG
            fprintf(console_out, "<TAB>");
#endif
            processTAB();
            break;

        case ascii::LF:    // Line Feed
            processLF();

            if (autoCR)
            {
                processCR();
            }

#if CONSOLE_LOG
            fprintf(console_out, "\n");
#endif
            break;

        case ascii::CR:    // Carriage Return
            processCR();

            if (autoLF)
            {
                processLF();
            }

            break;

        case ascii::CAN:   // Cancel.
            break;

        case ascii::ESC:   // Escape
            mode = Escape;
#if CONSOLE_LOG
            fprintf(console_out, "<ESC>");
#endif

            break;

        default:
            // if Printable character display it.
#if CONSOLE_LOG
            fprintf(console_out, "%c", ch);
#endif
            displayCharacter(ch);
            break;
        }
    }

    else if (mode == Escape)
    {
        // Assume we go back to Normal, so only the few that don't need to set the mode.
        mode = Normal;

        switch (ch)
        {
        case ascii::CAN: // CAN - Cancel
            // return to Normal mode, already set.
            break;

        case ascii::ESC: // Escape
            // From the ROM listing, stay in this mode.
            mode = Escape;
            break;

        // Cursor Functions

        case 'H': // Cursor Home
            PosX = PosY = 0;
            updated = true;
            break;

        case 'C': // Cursor Forward
            cursorForward();
            break;

        case 'D': // Cursor Backward
            processBS(); // same processing as cursor backward
            break;

        case 'B': // Cursor Down
            cursorDown();
            break;

        case 'A': // Cursor Up
            cursorUp();
            break;

        case 'I': // Reverse Index
            reverseIndex();
            break;

        case 'n': // Cursor Position Report
            cursorPositionReport();
            break;

        case 'j': // Save cursor position
            saveCursorPosition();
            break;

        case 'k': // Restore cursor position
            restoreCursorPosition();
            break;

        case 'Y': // Direct Cursor Addressing
            mode = DCA_1;
            break;

        // Erase and Editing

        case 'E': // Clear Display
            clearDisplay();
            break;

        case 'b': // Erase Beginning of Display
            eraseBOD();
            break;

        case 'J': // Erase to End of Page
            eraseEOP();
            break;

        case 'l': // Erase entire Line
            eraseEL();
            break;

        case 'o': // Erase Beginning Of Line
            eraseBOL();
            break;

        case 'K': // Erase To End Of Line
            eraseEOL();
            break;

        case 'L': // Insert Line
            insertLine();
            break;

        case 'M': // Delete Line
            deleteLine();
            break;

        case 'N': // Delete Character
            deleteChar();
            break;

        case '@': // Enter Insert Character Mode
            insertMode = true;
            break;

        case 'O': // Exit Insert Character Mode
            insertMode = false;
            break;

        // Configuration

        case 'z': // Reset To Power-Up Configuration
            reset();
            break;

        case 'r': // Modify the Baud Rate
            /// \todo - determine if we should support this.
            debugss(ssH19, ERROR, "Error Unimplemented Modify Baud\n");
            break;

        case 'x': // Set Mode
            mode = SetMode;
            break;

        case 'y': // Reset Mode
            mode = ResetMode;
            break;

        case '<': // Enter ANSI Mode
            /// \todo - implement ANSI mode.
            // ROM - just sets the mode
            debugss(ssH19, ERROR, "Error Entering ANSI mode - unsupported\n");
            break;

        // Modes of operation

        case '[': // Enter Hold Screen Mode
            holdScreen = true;
            break;

        case '\\': // Exit Hold Screen Mode
            holdScreen = false;
            break;

        case 'p': // Enter Reverse Video Mode
            reverseVideo = true;
            break;

        case 'q': // Exit Reverse Video Mode
            reverseVideo = false;
            break;

        case 'F': // Enter Graphics Mode
            graphicMode = true;
            break;

        case 'G': // Exit Graphics Mode
            graphicMode = false;
            break;

        case 't': // Enter Keypad Shifted Mode
            keypadShifted = true;
            break;

        case 'u': // Exit Keypad Shifted Mode
            // ROM - just sets the mode
            keypadShifted = false;
            break;

        case '=': // Enter Alternate Keypad Mode
            // ROM - just sets the mode
            keypadShifted = true;
            break;

        case '>': // Exit Alternate Keypad Mode
            // ROM - just sets the mode
            keypadShifted = false;
            break;

        // Additional Functions

        case '}': // Keyboard Disable
            /// \todo - determine whether to do this.
            keyboardEnabled = false;
            break;

        case '{': // Keyboard Enable
            keyboardEnabled = true;
            break;

        case 'v': // Wrap Around at End Of Line
            wrapEOL = true;
            break;

        case 'w': // Discard At End Of Line
            wrapEOL = false;
            break;

        case 'Z': // Identify as VT52 (Data: ESC / K)
            sendData(ascii::ESC);
            sendData('/');
            sendData('K');
            break;

        case ']': // Transmit 25th Line
            transmitLine25();
            break;

        case '#': // Transmit Page
            transmitPage();
            break;

        default:
            debugss(ssH19, WARNING, "Unhandled ESC: %d\n", ch);
            /// \todo - verify this is how the H19 ROM does it.
            break;
        }
    }

    else if (mode == SetMode)
    {
        mode = Normal;

        switch (ch)
        {
        case '1': // Enable 25th line
            // From the ROM, it erases line 25 on the enable, but here we erase on the disable.
            line25 = true;
            break;

        case '2': // No key click
            keyClick = true;
            break;

        case '3': // Hold screen mode
            holdScreen = true;
            break;

        case '4': // Block Cursor
            cursorBlock = true;
            updated = true;
            break;

        case '5': // Cursor Off
            cursorOff = true;
            updated = true;
            break;

        case '6': // Keypad Shifted
            keypadShifted = true;
            break;

        case '7': // Alternate Keypad mode
            altKeypadMode = true;
            break;

        case '8': // Auto LF
            autoLF = true;
            break;

        case '9': // Auto CR
            autoCR = true;
            break;

        default:
            /// \todo Need to process ch as if none of this happened...
            debugss(ssH19, WARNING, "Invalid set Mode: %c\n", ch);
            break;
        }
    }

    else if (mode == ResetMode)
    {
        mode = Normal;

        switch (ch)
        {
        case '1': // Disable 25th line
            eraseLine(rowsMain);
            line25 = false;
            updated = true;
            break;

        case '2': // key click
            keyClick = false;
            break;

        case '3': // Hold screen mode
            holdScreen = false;
            break;

        case '4': // Block Cursor
            cursorBlock = false;
            updated = true;
            break;

        case '5': // Cursor On
            cursorOff = false;
            updated = true;
            break;

        case '6': // Keypad Unshifted
            keypadShifted = false;
            break;

        case '7': // Exit Alternate Keypad mode
            altKeypadMode = false;
            break;

        case '8': // No Auto LF
            autoLF = false;
            break;

        case '9': // No Auto CR
            autoCR = false;
            break;

        default:
            /// \todo Need to process ch as if none of this happened...
            debugss(ssH19, WARNING, "Invalid reset Mode: %c\n", ch);
            break;
        }
    }

    else if (mode == DCA_1)
    {
        // From actual H19, once the line is specified, the cursor
        // immediately moves to that line, so no need to save the
        // position and wait for the entire command.
        /// \todo verify that this is the same on newer H19s.
        if (ch == ascii::CAN)
        {
            // cancel
            mode = Normal;
        }

        else
        {
            // \todo handle error conditions

            int pos = ch - 31;

            // verify valid Position
            if (((pos > 0) && (pos < (signed) rows)) || ((pos == (signed) rows) && (line25)))
            {
                PosY = pos - 1;
            }

            else
            {
                /// \todo check to see how a real h19 handles this.
                debugss(ssH19, INFO, "DCA invalid Y: %d\n", pos);
            }

            mode = DCA_2;
        }
    }

    else if (mode == DCA_2)
    {
        if (ch == ascii::CAN)
        {
            // cancel
            mode = Normal;
        }

        else
        {
            int pos = ch - 31;

            if ((pos > 0) && (pos < 81))
            {
                PosX = pos - 1;
            }

            else
            {
                PosX = (cols - 1);
            }

            updated = true;
            mode = Normal;
        }
    }
}

/// \brief Display character
///
void H19::displayCharacter(unsigned int ch)
{
    // when in graphic mode, use this lookup table to determine character to display,
    // note: although entries 0 to 31 are defined, they are not used, since the control
    // characters are specifically checked in the switch statement.
    static char graphicLookup[0x80] =
    {
        0,  1,  2,  3,  4,  5,  6,  7,  8,  9, 10, 11, 12, 13, 14,  15,
        16, 17, 18, 19, 20, 21, 22, 23, 24, 25, 26, 27, 28, 29, 30,  31,
        32, 33, 34, 35, 36, 37, 38, 39, 40, 41, 42, 43, 44, 45, 46,  47,
        48, 49, 50, 51, 52, 53, 54, 55, 56, 57, 58, 59, 60, 61, 62,  63,
        64, 65, 66, 67, 68, 69, 70, 71, 72, 73, 74, 75, 76, 77, 78,  79,
        80, 81, 82, 83, 84, 85, 86, 87, 88, 89, 90, 91, 92, 93, 127, 31,
        0,  1,  2,  3,  4,  5,  6,  7,  8,  9, 10, 11, 12, 13, 14,  15,
        16, 17, 18, 19, 20, 21, 22, 23, 24, 25, 26, 27, 28, 29, 30,  32
    };

    unsigned int symbol;

    if (graphicMode)
    {
        // Look up symbol
        symbol = graphicLookup[ch];
    }

    else
    {
        symbol = ch;
    }

    if (reverseVideo)
    {
        // set the high-bit for reverse video.
        symbol |= 0x80;
    }

    if (!((PosX <= cols) && (PosY < rows)))
    {
        debugss(ssH19, ERROR, "Invalid PosX, PosY: %d, %d\n", PosX, PosY);
        PosX = 0;
        PosY = 0;
    }

    if (insertMode)
    {
        for (unsigned int x = (cols - 1); x > PosX; --x)
        {
            screen[x][PosY] = screen[x - 1][PosY];
        }
    }

    if (PosX >= cols)
    {
        if (wrapEOL)
        {
            PosX = 0;

            if (PosY < (rowsMain - 1))
            {
                PosY++;
            }

            else if (PosY == (rowsMain - 1))
            {
                scroll();
            }

            else
            {
                // On a real H19, it just wraps back to column 0, and stays
                // on line 25 (24)
                assert(PosY == rowsMain);
            }
        }

        else
        {
            PosX = (cols - 1);
        }
    }

    screen[PosX][PosY] = symbol;
    PosX++;

    updated = true;
}

/// \brief Process Carriage Return
///
void H19::processCR()
{
    // check to possibly save the update.
    if (PosX)
    {
        PosX = 0;
        updated = true;
    }
}

/// \brief Process Line Feed
///
/// \todo - verify line 25 handling. make sure it doesn't clear line 25.
void H19::processLF()
{
    if (onLine25())
    {
        // On line 25, don't do anything
        return;
    }

    // Determine if we can just move down a row, or if we have to scroll.
    if (PosY < (rowsMain - 1))
    {
        ++PosY;
    }

    else
    {
        // must be line 24 - have to scroll.
        scroll();
    }

    updated = true;
}

/// \brief Process Backspace
///
void H19::processBS()
{
    if (PosX)
    {
        --PosX;
        updated = true;
    }
}

/// \brief Process TAB
///
void H19::processTAB()
{
    if (PosX < 72)
    {
        PosX += 8;
        // mask off the lower 3 bits to get the correct column.
        PosX &= 0xf8;
        updated = true;
    }

    else if (PosX < (cols - 1))
    {
        PosX++;
        updated = true;
    }
}

/// \brief Process Cursor Home
/// \todo - Determine how these function in relation to line 25..
void H19::cursorHome()
{
    if ((PosX) || (PosY))
    {
        PosX = PosY = 0;
        updated = true;
    }
}

/// \brief Process Cursor Forward
///
void H19::cursorForward()
{
    // ROM comment says that it will wrap around when at pos 80, but the code does not do that,
    // and verifying on a real H89, even with wrap-around enabled, the cursor will not wrap.
    if (PosX < (cols - 1))
    {
        ++PosX;
        updated = true;
    }
}

/// \brief Process cursor down
///
void H19::cursorDown()
{
    // ROM - Moves the cursor down one line on the display but does not cause a scroll past
    // the last line
    if (PosY < (rowsMain - 1))
    {
        ++PosY;
        updated = true;
    }
}

/// \brief Process cursor up
/// \todo determine if this is correct with line 25.
void H19::cursorUp()
{
    if (PosY)
    {
        --PosY;
        updated = true;
    }
}

///
/// \brief Process reverse index
///
void H19::reverseIndex()
{
    // Check for being on line 25
    if (onLine25())
    {
        /// \todo - is this right? or should we clear the 25th line?
        return;
    }

    // Make sure not first line
    if (PosY)
    {
        // simply move up a row
        --PosY;
    }

    else
    {
        // must be line 0 - have to scroll down.
        for (int y = (rowsMain - 1); y > 0; --y)
        {
            for (unsigned int x = 0; x < cols; ++x)
            {
                screen[x][y] = screen[x][y - 1];
            }
        }

        eraseLine(0);
    }

    updated = true;
}

/// \brief Process cursor position report
///
/// Send ESC Y <PosY+0x20> <PosX+0x20>
void H19::cursorPositionReport()
{
    // Send ESC Y <PosY+0x20> <PosX+0x20>
    sendData(ascii::ESC);
    sendData('Y');
    sendData(PosY + 0x20);
    sendData(PosX + 0x20);

}

/// \brief Process save cursor position
///
void H19::saveCursorPosition()
{
    SaveX = PosX;
    SaveY = PosY;
}

/// \brief process restore cursor position
///
void H19::restoreCursorPosition()
{
    PosX = SaveX;
    PosY = SaveY;

    updated = true;
}


// Erasing and Editing

/// \brief Clear Display
void H19::clearDisplay()
{
    // if on line 25, then only erase line 25
    if (onLine25())
    {
        eraseEL();
        /// \todo determine if 'PosX = 0' is needed.
    }

    else
    {
        for (unsigned int y = 0; y < rowsMain; ++y)
        {
            eraseLine(y);
        }

        PosX = PosY = 0;
    }

    updated = true;
}

/// \brief Erase to Beginning of display
///
void H19::eraseBOD()
{
    eraseBOL();

    // if on line 25 just erase to beginning of line
    if (!onLine25())
    {
        int y = PosY - 1;

        while (y >= 0)
        {
            eraseLine(y);
            --y;
        }
    }

    updated = true;
}

/// \brief Erase to End of Page
///
void H19::eraseEOP()
{
    eraseEOL();
    unsigned int y = PosY + 1;

    /// \todo what about line 25?
    while (y < rowsMain)
    {
        eraseLine(y);
        ++y;
    }

    updated = true;
}

/// \brief Erase to End of Line
///
void H19::eraseEL()
{
    eraseLine(PosY);

    updated = true;
}

/// \brief Erase to beginning of line
///
void H19::eraseBOL()
{
    int x = PosX;

    do
    {
        screen[x--][PosY] = ascii::SP;
    }
    while (x >= 0);

    updated = true;
}

/// \brief erase to end of line
///
void H19::eraseEOL()
{
    unsigned int x = PosX;

    do
    {
        screen[x++][PosY] = ascii::SP;
    }
    while (x < cols);

    updated = true;
}

/// \brief insert line
///
void H19::insertLine()
{
    /// \todo - Determine how the REAL H89 does this on Line 25, the ROM listing is not clear.
    /// - a real H19 messes up with either an insert or delete line on line 25.
    /// - note tested with early H19, newer H19 roms should have this fixed.
    for (unsigned int y = (rowsMain - 1); y > PosY; --y)
    {
        for (unsigned int x = 0; x < cols; ++x)
        {
            screen[x][y] = screen[x][y - 1];
        }
    }

    eraseLine(PosX);

    PosX = 0;

    updated = true;
}

/// \brief delete line
///
/// \todo - Determine how the REAL H89 does this on Line 25, the ROM listing is not clear.
/// - a real H19 messes up with either an insert or delete line on line 25.
/// - note tested with early H19, newer H19 roms should have this fixed.
void H19::deleteLine()
{
    // Go to the beginning of line
    PosX = 0;

    // move all the lines up.
    for (unsigned int y = PosY; y < (rowsMain - 1); ++y)
    {
        for (unsigned int x = 0; x < cols; ++x)
        {
            screen[x][y] = screen[x][y + 1];
        }
    }

    // clear line 24.
    eraseLine((rowsMain - 1));

    updated = true;
}

/// \brief delete character
///
void H19::deleteChar()
{
    // move all character in.
    for (unsigned int x = PosX; x < (cols - 1); x++)
    {
        screen[x][PosY] = screen[x + 1][PosY];
    }

    // clear the last column
    screen[cols - 1][PosY] = ascii::SP;

    updated = true;
}

void H19::eraseLine(unsigned int line)
{
    assert(line < rows);

    for (unsigned int x = 0; x < cols; ++x)
    {
        screen[x][line] = ascii::SP;
    }
};

/// \brief Process enable line 25.
///
void H19::processEnableLine25()
{
    // From the ROM, it erases line 25 on the enable, but here we erase on the disable.
    //
}

void H19::transmitLines(int start, int end)
{
    /// \todo verify this table.
    static char characterLookup[0x80] =
    {
        '`', 'a', 'b', 'c', 'd', 'e', 'f', 'g', 'h', 'i', 'j', 'k', 'l', 'm', 'n',  'o',
        'p', 'q', 'r', 's', 't', 'u', 'v', 'w', 'x', 'y', 'z', '{', '|', '}', '~',  '_',
        32, 33, 34, 35, 36, 37, 38, 39, 40, 41, 42, 43, 44, 45, 46, 47,
        48, 49, 50, 51, 52, 53, 54, 55, 56, 57, 58, 59, 60, 61, 62, 63,
        64, 65, 66, 67, 68, 69, 70, 71, 72, 73, 74, 75, 76, 77, 78, 79,
        80, 81, 82, 83, 84, 85, 86, 87, 88, 89, 90, 91, 92, 93, 94, 95,
        96, 97, 98, 99, 100, 101, 102, 103, 104, 105, 106, 107, 108, 109, 110, 111,
        112, 113, 114, 115, 116, 117, 118, 119, 120, 121, 122, 123, 124, 125, 126, '^'
    };

    bool reverse = false;
    bool graphics = false;
    unsigned char  ch;

    for (int line = start; line <= end; line++)
    {
        for (unsigned int col = 0; col < cols; col++)
        {
            bool newReverse = ((screen[col][line] & 0x80) == 0x80);
            /// \todo determine if we should only change for lower case and graphics characters.
            ///
            bool newGraphics = (((screen[col][line] & 0x7f) < 0x20) ||
                                ((screen[col][line] & 0x7f) == 0x7f));

            /// \todo - determine which mode a real H19 sends first.
            if (newReverse != reverse)
            {
                if (newReverse)
                {
                    /// \todo - if in ANSI mode, must send the ANSI codes
                    // send turn on reverse
                    sendData(ascii::ESC);
                    sendData('p');
                }

                else
                {
                    // send turn off reverse
                    sendData(ascii::ESC);
                    sendData('q');
                }

                reverse = newReverse;
            }

            if (newGraphics != graphics)
            {
                if (newGraphics)
                {
                    // send turn on graphics
                    sendData(ascii::ESC);
                    sendData('F');
                }

                else
                {
                    // send turn off graphics
                    sendData(ascii::ESC);
                    sendData('G');
                }

                graphics = newGraphics;
            }

            // mask off the inverse video.
            ch = screen[col][line] & 0x7f;

            if (graphics)
            {
                // look up the character
                ch = characterLookup[ch];
            }

            // finally send the character.
            sendData(ch);
        }
    }

    /// \todo - determine if we need to turn off inverse and graphics if they are on.
}

/// \brief Process transmit line 25
///
void H19::transmitLine25()
{
    // if line 25 is not enabled, only send CR and ring bell.
    if (line25)
    {
        // Transmit line 25.
        transmitLines(rowsMain, rowsMain);
    }

    sendData(ascii::CR);
    bell();
}

/// \brief Process transmit page
///
void H19::transmitPage()
{
    transmitLines(0, rowsMain - 1);
    sendData(ascii::CR);
    bell();
}

/// \brief Ring the H19 bell.
///
/// \todo implement bell
void H19::bell(void)
{

}

unsigned int H19::getBaudRate()
{
    /// \todo get this from the dip switch.
    return (9600);
}


void H19::reshape(int w, int h)
{
    glViewport(0, 0, (GLsizei) w, (GLsizei) h);

    glMatrixMode(GL_PROJECTION);
    glLoadIdentity();
    glOrtho(0.0, w, 0.0, h, -1.0, 1.0);

    glMatrixMode(GL_MODELVIEW);
    glClearColor(0.0f, 0.0f, 0.0f, 0.9f);
    glClear(GL_COLOR_BUFFER_BIT);
}

void H19::timer(int i)
{
    static int count = 0;

    if ((++count % 60) == 0)
    {
        fflush(console_out);
    }

    // Tell glut to redisplay the scene:
    if (h19->checkUpdated())
    {
        glutPostRedisplay();
    }

    // Need to call this method again after the desired amount of time has passed:
    glutTimerFunc(screenRefresh, timer, i);
}

void H19::keyboard(unsigned char key, int x, int y)
{
    h19->keypress(key);
}

void H19::special(int key, int x, int y)
{
    // NOTE: GLUT has already differentiated exact keystrokes
    // based on modern keyboard standards. Here we just encode
    // the modern key codes into something convenient to use
    // in the H19 class.
    switch (key)
    {
    case GLUT_KEY_F1:
        h19->keypress('S' | 0x80);
        break;

    case GLUT_KEY_F2:
        h19->keypress('T' | 0x80);
        break;

    case GLUT_KEY_F3:
        h19->keypress('U' | 0x80);
        break;

    case GLUT_KEY_F4:
        h19->keypress('V' | 0x80);
        break;

    case GLUT_KEY_F5:
        h19->keypress('W' | 0x80);
        break;

    case GLUT_KEY_F6:
        h19->keypress('P' | 0x80);
        break;

    case GLUT_KEY_F7:
        h19->keypress('Q' | 0x80);
        break;

    case GLUT_KEY_F8:
        h19->keypress('R' | 0x80);
        break;

    case GLUT_KEY_HOME:
        h19->keypress('H' | 0x80);
        break;

    case GLUT_KEY_UP:
        h19->keypress('A' | 0x80);
        break;

    case GLUT_KEY_DOWN:
        h19->keypress('B' | 0x80);
        break;

    case GLUT_KEY_LEFT:
        h19->keypress('D' | 0x80);
        break;

    case GLUT_KEY_RIGHT:
        h19->keypress('C' | 0x80);
        break;

    default:
        break;
    }
}

void H19::glDisplay()
{
    h19->display();
}


void H19::run()
{
    int dummy_argc = 1;
    char *dummy_argv = (char *)"dummy";

    glutInit(&dummy_argc, &dummy_argv);
    glutInitDisplayMode(GLUT_DOUBLE | GLUT_RGBA);
    glutInitWindowSize(640, 500);
    glutInitWindowPosition(500, 100);
    glutCreateWindow((char *) "Virtual Heathkit H-89 All-in-One Computer");

    glClearColor(0.0f, 0.0f, 0.0f, 0.9f);
    //glutInitDisplayMode(GLUT_DOUBLE | GLUT_RGBA);
    glBlendFunc(GL_ONE, GL_ONE);
    //glBlendEquation(GL_FUNC_ADD);
    glBlendColor(0.5, 0.5, 0.5, 0.9);
    //glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);
    glEnable(GL_BLEND);

    glShadeModel(GL_FLAT);
    initGl();
    glutReshapeFunc(reshape);
    glutKeyboardFunc(keyboard);
    glutSpecialFunc(special);
    glutDisplayFunc(glDisplay);
    glutTimerFunc(screenRefresh, timer, 1);
    glutIgnoreKeyRepeat(1);

    glutMainLoop();
}
