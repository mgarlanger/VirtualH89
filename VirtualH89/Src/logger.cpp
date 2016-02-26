/// \file logger.cpp
///
/// \date May 6, 2009
/// \author Mark Garlanger
///

#include "logger.h"


unsigned debugLevel[ssMax];


logger::logger():  printToFile(false),
                   printToScreen(false),
                   logFile(NULL)
{

}

logger::~logger()
{

}

void
setDebugLevel()
{
    logLevel defaultLevel = ERROR;

    debugLevel[ssH89]                 = defaultLevel; // The overall H89
    debugLevel[ssMEM]                 = defaultLevel; // Memory class
    debugLevel[ssRAM]                 = defaultLevel; // RAM accesses
    debugLevel[ssROM]                 = defaultLevel; // ROM accesses
    debugLevel[ssZ80]                 = defaultLevel; // Z80 CPU
    debugLevel[ssInterruptController] = defaultLevel; // Interrupt Controller
    debugLevel[ssAddressBus]          = defaultLevel; // Address Bus
    debugLevel[ssIO]                  = defaultLevel; // I/O ports
    debugLevel[ssH17]                 = defaultLevel; // H17 controller
    debugLevel[ssH37]                 = defaultLevel; // H37 controller
    debugLevel[ssH47]                 = defaultLevel; // H47 controller
    debugLevel[ssH67]                 = defaultLevel; // H67 controller
    debugLevel[ssDiskDrive]           = defaultLevel; // Floppy disks
    debugLevel[ssH17_1]               = defaultLevel;
    debugLevel[ssH17_4]               = defaultLevel;
    debugLevel[ssH47Drive]            = defaultLevel;
    debugLevel[ssConsole]             = defaultLevel; // Console
    debugLevel[ssH19]                 = defaultLevel; // H19 Terminal
    debugLevel[ss8250]                = defaultLevel; // 8250 Serial Port
    debugLevel[ssTimer]               = defaultLevel; // 2 mSec Timer
    debugLevel[ssWallClock]           = defaultLevel; // Wall Clock.
    // debugLevel[ssDiskDrive]  = defaultLevel;  // Disk Drive
    debugLevel[ssFloppyDisk]          = defaultLevel; // Floppy Disk
    debugLevel[ssGpp]                 = defaultLevel; // General Purpose Port
    debugLevel[ssParallel]            = defaultLevel; // Parallel Port Interface (Z47)
    debugLevel[ssStdioConsole]        = defaultLevel;
    debugLevel[ssMMS77316]            = defaultLevel;
    debugLevel[ssWD1797]              = defaultLevel;
    debugLevel[ssGenericFloppyDrive]  = defaultLevel;
    debugLevel[ssRawFloppyImage]      = defaultLevel;
    debugLevel[ssMMS77320]            = defaultLevel;
    debugLevel[ssGenericSASIDrive]    = defaultLevel;
    debugLevel[ssHostFileBdos]   = defaultLevel;
    debugLevel[ssCPNetDevice]   = defaultLevel;
    debugLevel[ssSectorFloppyImage]   = defaultLevel;
}

void
setDebug(subSystems ss, logLevel level)
{
    debugLevel[ss] = level;
}
