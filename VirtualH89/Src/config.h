/// \file config.h
///
/// \date Mar 7, 2009
/// \author Mark Garlanger
///

#ifndef CONFIG_H_
#define CONFIG_H_


/// \TODO this file should be removed and make all configuration run-time in the configuration file.
/// \todo Determine what needs to be a configuration variable, and what can be removed.
// #define BUS_8080

// Need if dumping all the Z80 logs. Otherwise system is too slow to process
// all the output -  actually 20x
#define TEN_X_SLOWER 0

#endif // CONFIG_H_
