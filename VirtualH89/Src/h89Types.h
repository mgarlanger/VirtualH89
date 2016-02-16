/// \file h89Types.h
///
/// \date Mar 7, 2009
/// \author Mark Garlanger
///
///

#ifndef H89_TYPES_H_
#define H89_TYPES_H_

#include <inttypes.h>

///
/// \typedef WORD
///
/// 16-bit unsigned integer
///
typedef uint16_t    WORD;

///
/// \typedef SWORD
///
/// 16-bit signed integer
///
typedef int16_t     SWORD;

///
/// \typedef BYTE
///
/// 8-bit unsigned integer
///
typedef uint8_t     BYTE;

///
/// \typedef SBYTE
///
/// 8-bit signed integer
///
typedef int8_t      SBYTE;

///
/// \struct RP
///
/// \brief Register Pair
///
/// A union to allow the register to appear as an unsigned word, a signed word, 2 unsigned
/// bytes, or 2 signed bytes. All without any special casting/conversions.
///
struct RP
{
    union
    {
        /// Option 1, register appears to be 2 unsigned bytes.
        struct
        {
            BYTE lo;
            BYTE hi;
        };

        /// Option 2, register appears to be 2 signed bytes.
        struct
        {
            SBYTE slo;
            SBYTE shi;
        };

        /// Option 3, register appears to be an unsigned word.
        WORD val;

        /// Option 4, register appears to be a signed word.
        SWORD sval;
    };
};

#endif    // H89_TYPES_H_
