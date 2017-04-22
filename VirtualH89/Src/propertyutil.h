// Java-Style Properties in C++
//
// (c) Paul D. Senzee
// Senzee 5
// http://senzee.blogspot.com

#ifndef _PROPERTYUTIL_H
#define _PROPERTYUTIL_H

/// \cond
#include <map>
#include <string>
#include <iostream>
#include <vector>
/// \endcond

class PropertyUtil
{
    // enum { DEBUG = 0 };

  public:

    typedef std::map<std::string, std::string> PropertyMapT;
    typedef PropertyMapT::value_type value_type;
    typedef PropertyMapT::iterator iterator;

    static void read(const char*   filename,
                     PropertyMapT& map);
    static void read(std::istream& is,
                     PropertyMapT& map);
    static void write(const char*   filename,
                      PropertyMapT& map,
                      const char*   header = nullptr);
    static void write(std::ostream& os,
                      PropertyMapT& map,
                      const char*   header = nullptr);
    static void print(std::ostream& os,
                      PropertyMapT& map);
    static std::vector<std::string> splitArgs(std::string prop);
    static std::string combineArgs(std::vector<std::string> args,
                                   int                      start = 0);
    static std::vector<std::string> shiftArgs(std::vector<std::string> args,
                                              int                      start);
    static std::string sprintf(const char* fmt ...);

  private:

    static inline char m_hex(int nibble)
    {
        static const char* digits = "0123456789ABCDEF";
        return digits[nibble & 0xf];
    }
};

#endif // _PROPERTYUTIL_H
