/// \file logger.h
///
/// \date May 6, 2009
/// \author Mark Garlanger
///

#ifndef LOGGER_H_
#define LOGGER_H_

#include <cstdio>

#include "WallClock.h"

extern FILE* log_out;

extern FILE* console_out;

/// \todo - make the logger a separate thread.
/// \todo - have logger collapse repeated lines.
/// \todo - have logger create it's own file instead of being defined in main.h
class logger
{
  public:

  private:
    logger();
    ~logger();
    bool  printToFile;
    bool  printToScreen;
    FILE* logFile;

};

#define DEBUG 1
#define DEBUG_TO_FILE 1

///
enum subSystems
{
    ssH89,
    ssMEM,
    ssRAM,
    ssROM,
    ssZ80,
    ssInterruptController,
    ssH37InterruptController,
    ssAddressBus,
    ssIO,
    ssH17,
    ssH37,
    ssH47,
    ssH67,
    ssDiskDrive,
    ssH17_1,
    ssH17_4,
    ssH47Drive,
    ssConsole,
    ssH19,
    ss8250,
    ssSerial,
    ssTimer,
    ssWallClock,
    ssFloppyDisk,
    ssGpp,
    ssParallel,
    ssStdioConsole,
    ssMMS77316,
    ssWD1797,
    ssGenericFloppyDrive,
    ssRawFloppyImage,
    ssMMS77320,
    ssGenericSASIDrive,
    ssHostFileBdos,
    ssCPNetDevice,
    ssSectorFloppyImage,
    ssMax
};

///
///  Logging Levels
///
///  FATAL   - can't be turned off if logging is enabled, an error condition that should not
///            occur.
///  ERROR   - maskable errors
///  WARNING - warning that something is probably wrong
///  INFO    - General info about object creation or other infrequent events.
///  VERBOSE -
///  ALL     - Detailed logs that will generally generate MASSIVE amounts of logs, for example
///            in the ssZ80, this will enable per instruction dumping of all registers - on
///            the order of 500,000 logs per second.
///
enum logLevel
{
    FATAL   = 0,
    ERROR   = 10,
    WARNING = 20,
    INFO    = 30,
    VERBOSE = 40,
    ALL     = 100
};

#if DEBUG

#if DEBUG_TO_FILE
// #define debug(cond, ...)   if (cond) { printf(__VA_ARGS__); }

#define debug(...)         {if (log_out){fprintf(log_out, __VA_ARGS__); }}

extern void __debugss(enum subSystems, enum logLevel, const char* functionName, const char* fmt,
                      ...);

#define debugss(subsys, level, args ...)                     \
    if (level <= debugLevel[subsys])                         \
    {                                                        \
        __debugss(subsys, level, __PRETTY_FUNCTION__, args); \
    }


// nts - No TimeStamp
#define debugss_nts(subsys, level, args ...) \
    {                                        \
        if (level <= debugLevel[subsys])     \
        {                                    \
            fprintf(log_out, args);          \
        }                                    \
    }

#define chkdebuglevel(subsys, level)  ((level <= debugLevel[subsys]))

#else // if !DEBUG_TO_FILE
#define cond_debug(cond, ...)   if (cond){printf(__VA_ARGS__); }
#define debug(args ...)              {printf(args); }
#endif // DEBUG_TO_FILE
#else   // if !DEBUG
#define debug(args ...)
#define debugss(subsys, level, args ...)
#define debugss_nts(subsys, level, args ...)
#define chkdebuglevel(subsys, level) (false)
#endif // DEBUG

extern unsigned debugLevel[ssMax];

void setDebugLevel();

void setDebug(subSystems ss,
              logLevel   level);


#endif // LOGGER_H_
