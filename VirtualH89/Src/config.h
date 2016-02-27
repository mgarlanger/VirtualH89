/// \file config.h
///
/// \date Mar 7, 2009
/// \author Mark Garlanger
///

#ifndef CONFIG_H_
#define CONFIG_H_

/// \todo Determine what needs to be a configuration variable, and what can be removed.
// #define BUS_8080

// h17
#define RAW 1

// z80
#define TWOMSEC 1
#define USE_TABLE 1

// Need if dumping all the Z80 logs. Otherwise system is too slow to process
// all the output -  actually 20x
#define TEN_X_SLOWER 0

#define MTR90 1

#define Z37 0
#define Z47 1

#define CONSOLE_LOG 1

#endif // CONFIG_H_
