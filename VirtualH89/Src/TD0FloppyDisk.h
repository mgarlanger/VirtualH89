///
///  \file TD0FloppyDisk.h
///
///  \author Mark Garlanger
///  \date May 2, 2016
///

#ifndef TD0FLOPPYDISK_H_
#define TD0FLOPPYDISK_H_

#include "GenericFloppyDisk.h"
#include "SoftSectoredDisk.h"
/// \cond
#include <vector>
#include <memory>
/// \endcond

class DiskSide;
class Track;
class Sector;

class TD0FloppyDisk: public SoftSectoredDisk
{
  public:
    TD0FloppyDisk(std::vector<std::string> argv);
    virtual ~TD0FloppyDisk();

    static std::shared_ptr<GenericFloppyDisk> getDiskette(std::vector<std::string> argv);

  private:

    // int           gapLen_m;
    // int           indexGapLen_m;
    // unsigned long writePos_m;
    // bool          trackWrite_m;

  protected:
    bool readTD0(const char* name);

    // LZSS parameters
    static const int     SB_SIZE   = 4096; // Size of Ring buffer
    static const int     LA_SIZE   = 60;   // Size of Look-ahead buffer
    static const int     THRESHOLD = 2;    // Minimum match for compress

    // Huffman coding parameters
    static const int     N_CHAR    = (256 - THRESHOLD + LA_SIZE); // Character code (= 0..N_CHAR-1)
    static const int     T_SIZE    = (N_CHAR * 2 - 1);            // Size of table
    static const int     ROOT      = (T_SIZE - 1);                // Root position
    static const int     MAX_FREQ  = 0x8000;                      // Update when cumulative frequency
                                                                  // reaches this value

    unsigned long int    pos_m     = 0;
    BYTE*                buf;
    size_t               size_m;

    unsigned int         parent[T_SIZE + N_CHAR];          // parent nodes (0..T-1) and leaf positions (rest)
    unsigned int         son[T_SIZE];                      // pointers to child nodes (son[], son[]+1)
    unsigned int         freq[T_SIZE + 1];                 // frequency table

    unsigned int         bits_m, bitBuff_m;                // buffered bit count and left-aligned bit buffer
    unsigned int         GBr;                              // Ring buffer position
    unsigned int         GBi;                              // Decoder index
    unsigned int         GBj;                              // Decoder index
    unsigned int         GBk;                              // Decoder index

    unsigned char        ring_buff[SB_SIZE + LA_SIZE - 1]; // text buffer for match strings

    bool                 gbState_m;                        // Decoder state
    bool                 eof_m;                            // End-of-file indicator
    bool                 advanceCompression_m;

    static unsigned char d_code[256];
    static unsigned char d_len[];

    void initDecompress();
    void update(int c);
    unsigned GetChar();
    unsigned GetBit();
    unsigned GetByte();
    unsigned DecodeChar();
    unsigned DecodePosition();
    unsigned getbyte();
    unsigned getword();

};

#endif //  TD0FLOPPYDISK_H_
