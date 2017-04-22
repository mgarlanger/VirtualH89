/// \file HostFileBdos.h
///
///  Virtual access to host filesystem via CP/Net-style I/O device.
///
///  \date Feb 19, 2016
///  \author Douglas Miller
///

#ifndef HOSTFILEBDOS_H_
#define HOSTFILEBDOS_H_

#include "NetworkServer.h"
#include "propertyutil.h"

/// \cond
#include <dirent.h>
#include <string>
/// \endcond

#define BDOS_FUNC(name)                                               \
    int name(uint8_t * msgbuf, int len);                              \
    static int name(HostFileBdos * thus, uint8_t * msgbuf, int len) { \
        return thus->name(msgbuf, len);                               \
    }

// An instance of HostFileBdos represents a client-server pair.
// This is typically a dedicated network connection (e.g. socket),
// but may also be an informal pairing such as a virtual machine's
// "connection" to the native host.

class HostFileBdos: public NetworkServer
{
  public:
    HostFileBdos(PropertyUtil::PropertyMapT& props,
                 std::vector<std::string> args, uint8_t srvId, uint8_t cltId);
    virtual ~HostFileBdos() override;

    virtual int checkRecvMsg(uint8_t clientId, uint8_t* msgbuf, int len) override;
    virtual int sendMsg(uint8_t* msgbuf, int len) override;
  private:
    char*   dir;
    uint8_t serverId;
    uint8_t clientId;

    struct fcb
    {
        uint8_t drv;
        uint8_t name[11];
        uint8_t ext;
        uint8_t s1[2];
        uint8_t rc;
        union
        {
            uint8_t d0[16];
            struct
            {
                uint16_t fd;
                uint16_t fd_;
                int      oflags;
            };
        };
        uint8_t cr;
        uint8_t rr[3];
    } __attribute__((packed));

    struct fcbdate
    {
        uint16_t date;
        uint8_t  hour, min;
    } __attribute__((packed));

    struct cpmdate
    {
        uint16_t date;
        uint8_t  hour, min, sec;
    } __attribute__((packed));

    struct sfcbs
    {
        struct fcbdate atim;
        struct fcbdate utim;
        uint8_t        pwmode;
        uint8_t        reserved;
    } __attribute__((packed));

    struct sfcb
    {
        uint8_t      drv; // always 0x21
        struct sfcbs fcbs[3];
        uint8_t      reserved;
    } __attribute__((packed));

    struct dirl
    {
        uint8_t        drv;    // always 0x20
        uint8_t        name[11];
        uint8_t        ext;    // mode byte
        uint8_t        s1[2];  // 0x00 0x00
        uint8_t        rc;     // 0x00
        uint8_t        pwd[8]; // password
        struct fcbdate ctim;
        struct fcbdate utim;
    } __attribute__((packed));

    struct search
    {
        int     iter;
        int     lastit;
        int     endit;
        uint8_t maxdc; // 3 or 4, depending on timestamps (SFCBs)
        uint8_t drv;
        uint8_t usr;
        uint8_t ext;
        uint8_t full;
        uint8_t cpm3;
        off_t   size;
        struct find
        {
            DIR* dir;
            char pat[16];
            char path[1024];
            int  dirlen;
        } find;
    };

    struct dpb
    {
        uint16_t spt;
        uint8_t  bsh;
        uint8_t  blm;
        uint8_t  exm;
        uint16_t dsm;
        uint16_t drm;
        uint8_t  al0;
        uint8_t  al1;
        uint16_t cks;
        uint16_t off;
    } __attribute__((packed));

    static const int     DEF_BLS_SH  = 14;  // 2^14 = 16K
    static const int     DEF_BLS     = (1 << DEF_BLS_SH);
    static const int     DEF_NBLOCKS = 128; // keep alloc vec small, disk size 2M
    static const int     DEF_NFILE   = 32;  // Probably never need even 8.
    static const uint8_t dirMode     = 0b01100001;

    static int(*bdosFunctions[256])(HostFileBdos*, uint8_t*, int);

    uint16_t             curDma;
    void*                curDmaReal;
    uint16_t             curDsk;
    uint16_t             curUsr;
    uint16_t             curROVec;
    uint16_t             curLogVec;
    struct search        curSearch;
    struct dpb           curDpb;
    int                  phyExt; // phy ext size, recs
    int                  openFiles[DEF_NFILE];
    char                 fileName[sizeof((struct dirent*) 0)->d_name];
    char                 pathName[PATH_MAX];
    struct sfcb          sFcb;

    BDOS_FUNC(getDPB);
    BDOS_FUNC(writeProt);
    BDOS_FUNC(resetDrive);
    BDOS_FUNC(getROVec);
    BDOS_FUNC(getAllocVec);
    BDOS_FUNC(getLoginVec);
    BDOS_FUNC(selectDisk);
    BDOS_FUNC(openFile);
    BDOS_FUNC(closeFile);
    BDOS_FUNC(searchNext);
    BDOS_FUNC(searchFirst);
    BDOS_FUNC(deleteFile);
    BDOS_FUNC(readSeq);
    BDOS_FUNC(writeSeq);
    BDOS_FUNC(createFile);
    BDOS_FUNC(renameFile);
    BDOS_FUNC(readRand);
    BDOS_FUNC(writeRand);
    BDOS_FUNC(setRandRec);
    BDOS_FUNC(compFileSize);
    BDOS_FUNC(setFileAttrs);
    BDOS_FUNC(accessDrive);
    BDOS_FUNC(freeDrive);
    BDOS_FUNC(writeRandZF);
    BDOS_FUNC(lockRec);
    BDOS_FUNC(unlockRec);
    BDOS_FUNC(getFreeSp);
    BDOS_FUNC(flushBuf);
    BDOS_FUNC(freeBlks);
    BDOS_FUNC(truncFile);
    BDOS_FUNC(login);
    BDOS_FUNC(logoff);
    BDOS_FUNC(setCompAttrs);
    BDOS_FUNC(getServCfg);
    BDOS_FUNC(setDefPwd);
    BDOS_FUNC(getDirLab);
    BDOS_FUNC(readFStamps);
    BDOS_FUNC(getTime);

    // Utility functions
    int cpmDrive(char* buf, int drive);
    int cpmFilename(char* buf, int user, char* file);
    int cpmPath(char* buf, int drive, int user, char* file);
    char* cpmNameFound(struct search::find* find);
    char* cpmPathFound(struct search::find* find);
    void cpmFindInit(struct search::find* find, int drive, char* pattern);
    const char* cpmFind(struct search::find* find);
    void getFileName(char* dst, struct fcb* fcb);
    void getAmbFileName(char* dst, struct fcb* fcb, uint8_t usr);
    void copyOutDir(uint8_t* dma, const char* name);
    int fullSearch(uint8_t* dirbuf, struct search* search, const char* ff);
    int copyOutSearch(uint8_t* buf, const char* name);
    const char* commonSearch(struct search* search);
    const char* doSearch(struct search* search);
    const char* startSearch(struct fcb* fcb, struct search* search, uint8_t u);
    void seekFile(struct fcb* fcb);
    int openFileFcb(struct fcb* fcb, uint8_t u);
    int newFileFcb();
    int locFileFcb(struct fcb* fcb);
    void putFileFcb(struct fcb* fcb, int ix, int fd);
    int getFileFcb(struct fcb* fcb);
    int closeFileFcb(struct fcb* fcb);
    int makeFileFcb(struct fcb* fcb, uint8_t u);
    void unix2cpmdate(time_t unx, struct cpmdate* cpm);

};

#endif // HOSTFILEBDOS_H_
