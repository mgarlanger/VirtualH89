/// \file config.h
///
/// \date Mar 7, 2009
/// \author Mark Garlanger
///

#ifndef CONFIG_H_
#define CONFIG_H_

/// \todo Determine what needs to be a configuration variable, and what can be removed.

#define USE_PTHREAD 1
#define OGL 1
#define CPM 0
#define QTGL 0
#if (!OGL)
#define __WXOSX_COCOA__ 1
#define WXUSINGDLL 1
//#define wxABORT_ON_CONFIG_ERROR 0
#endif

#define WXWID 1

#define CNTL_C
//#define BUS_8080

// h17
#define RAW 1

// z80
#define TWOMSEC 1
#define NEW_TICKS 1
#define USE_TABLE 1
#define INDIVIDUAL_FUNCTIONS 0

// Need if dumping all the Z80 logs. Otherwise system is too slow to process all the output
#define TEN_X_SLOWER 0

#define PREFIX 1

#define MTR90 1

#define Z37 0
#define Z47 1

#define CONSOLE_LOG 1

#endif // CONFIG_H_
