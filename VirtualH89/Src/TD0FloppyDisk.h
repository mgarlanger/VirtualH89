///
///  \file TD0FloppyDisk.h
///
///  \author Mark Garlanger
///  \date May 2, 2016
///

#ifndef TD0FLOPPYDISK_H_
#define TD0FLOPPYDISK_H_

#include "GenericFloppyDisk.h"

#include <vector>
#include <memory>

class Track;
class Sector;

class TD0FloppyDisk: public GenericFloppyDisk
{
  public:
    TD0FloppyDisk(std::vector<std::string> argv);
    virtual ~TD0FloppyDisk();

    virtual bool readData(BYTE track,
                          BYTE side,
                          BYTE sector,
                          int  inSector,
                          int& data);
    virtual bool writeData(BYTE track,
                           BYTE side,
                           BYTE sector,
                           int  inSector,
                           BYTE data,
                           bool dataReady,
                           int& result);
    virtual BYTE getMaxSectors(BYTE sides,
                               BYTE track);
    virtual bool isReady();
    virtual void eject(const char* name);
    virtual void dump(void);
    virtual std::string getMediaName();
    static std::shared_ptr<GenericFloppyDisk> getDiskette(std::vector<std::string> argv);

  private:

    const char*                           imageName_m;
    static const unsigned int             maxHeads_c = 2;

    std::vector<std::shared_ptr<Track> >  tracks_m[maxHeads_c];

    std::shared_ptr<Sector>               curSector_m;
    int                                   dataPos_m;
    int                                   sectorLength_m;
    BYTE                                  secLenCode_m;
    bool                                  ready_m;

    // bool                                  hypoTrack_m;  // ST media in DT drive
    // bool                                  hyperTrack_m; // DT media in ST drive

    // bool          interlaced_m;
    // int           mediaLat_m;
    // BYTE          secLenCode_m;
    // int           gapLen_m;
    // int           indexGapLen_m;
    // unsigned long writePos_m;
    // bool          trackWrite_m;
    // int           dataPos_m;
    // int           dataLen_m;

    // bool checkHeader(BYTE* buf, int n);
    // bool cacheSector(int side, int track, int sector);

  protected:
    bool readTD0(const char* name);
    bool findSector(int side, int track, int sector);

    // LZSS parameters
    static const int     SBSIZE    = 4096; // Size of Ring buffer
    static const int     LASIZE    = 60;   // Size of Look-ahead buffer
    static const int     THRESHOLD = 2;    // Minimum match for compress

    // Huffman coding parameters
    static const int     N_CHAR    = (256 - THRESHOLD + LASIZE); // Character code (= 0..N_CHAR-1)
    static const int     TSIZE     = (N_CHAR * 2 - 1);           // Size of table
    static const int     ROOT      = (TSIZE - 1);                // Root position
    static const int     MAX_FREQ  = 0x8000;                     // Update when cumulative frequency
                                                                 // reaches this value

    unsigned long int    pos       = 0;
    BYTE*                buf;
    size_t               size;

    unsigned int         parent[TSIZE + N_CHAR];         // parent nodes (0..T-1) and leaf positions (rest)
    unsigned int         son[TSIZE];                     // pointers to child nodes (son[], son[]+1)
    unsigned int         freq[TSIZE + 1];                // frequency table

    unsigned int         Bits, Bitbuff;                  // buffered bit count and left-aligned bit buffer
    unsigned int         GBcheck;                        // Getbyte check down-counter
    unsigned int         GBr;                            // Ring buffer position
    unsigned int         GBi;                            // Decoder index
    unsigned int         GBj;                            // Decoder index
    unsigned int         GBk;                            // Decoder index

    unsigned char        ring_buff[SBSIZE + LASIZE - 1]; // text buffer for match strings

    bool                 GBstate;                        // Decoder state
    bool                 Eof;                            // End-of-file indicator
    bool                 advanceCompression_m;

    static unsigned char d_code[256];
    static unsigned char d_len[];

    void init_decompress();
    void update(int c);
    unsigned GetChar();
    unsigned GetBit();
    unsigned GetByte();
    unsigned DecodeChar();
    unsigned DecodePosition();
    int getbyte();
    unsigned getword();

};


#endif //  TD0FLOPPYDISK_H_
