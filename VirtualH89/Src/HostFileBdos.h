/// \file HostFileBdos.h
///
///  Virtual access to host filesystem via CP/Net-style I/O device.
///
///  \date Feb 19, 2016
///  \author Douglas Miller
///

#ifndef HOSTFILEBDOS_H_
#define HOSTFILEBDOS_H_

#include "config.h"
#include "h89Types.h"
#include "NetworkServer.h"
#include "propertyutil.h"

#include <dirent.h>
#include <string>

#define BDOS_FUNC(name)                                            \
    int name(BYTE * msgbuf, int len);                              \
    static int name(HostFileBdos * thus, BYTE * msgbuf, int len) { \
        return thus->name(msgbuf, len);                            \
    }

class HostFileBdos: public NetworkServer
{
  public:
    HostFileBdos(PropertyUtil::PropertyMapT& props,
                 std::vector<std::string> args, BYTE srvId);
    virtual ~HostFileBdos();

    virtual int checkRecvMsg(BYTE clientId, BYTE* msgbuf, int len);
    virtual int sendMsg(BYTE* msgbuf, int len);

  private:
    const char* dir;
    BYTE        serverId;

    struct fcb
    {
        BYTE drv;
        BYTE name[11];
        BYTE ext;
        BYTE s1[2];
        BYTE rc;
        union
        {
            BYTE d0[16];
            struct
            {
                int fd;
                int oflags;
            };
        };
        BYTE cr;
        BYTE rr[3];
    } __attribute__((packed));

    struct search
    {
        int   iter;
        BYTE  drv;
        BYTE  usr;
        BYTE  ext;
        off_t size;
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
        WORD spt;
        BYTE bsh;
        BYTE blm;
        BYTE exm;
        WORD dsm;
        WORD drm;
        BYTE al0;
        BYTE al1;
        WORD cks;
        WORD off;
    } __attribute__((packed));

    static const int DEF_BLS_SH  = 14;  // 2^14 = 16K
    static const int DEF_BLS     = (1 << DEF_BLS_SH);
    static const int DEF_NBLOCKS = 128; // keep alloc vec small, disk size 2M

    static int (*bdosFunctions[256])(HostFileBdos*, BYTE*, int);

    WORD             curDma;
    void*            curDmaReal;
    WORD             curDsk;
    WORD             curUsr;
    WORD             curROVec;
    WORD             curLogVec;
    struct search    curSearch;
    struct dpb       curDpb;

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
    BDOS_FUNC(unlockRec);
    BDOS_FUNC(login);
    BDOS_FUNC(logoff);
    BDOS_FUNC(setCompAttrs);
    BDOS_FUNC(getServCfg);
    BDOS_FUNC(setDefPwd);
    BDOS_FUNC(getTime);

    // Utility functions
    int cpmDrive(char* buf, int drive);
    int cpmFilename(char* buf, int user, char* file);
    int cpmPath(char* buf, int drive, int user, char* file);
    char* cpmNameFound(struct search::find* find);
    char* cpmPathFound(struct search::find* find);
    char* cpmFindInit(struct search::find* find);
    char* cpmFind(struct search::find* find, int drive, char* pattern);
    void getFileName(char* dst, struct fcb* fcb);
    void getAmbFileName(char* dst, struct fcb* fcb, uint8_t usr);
    void copyOutDir(BYTE* dma, char* name);
    void copyOutSearch(BYTE* buf, char* name);
    char* commonSearch(struct search* search, char* pat);
    char* doSearch(struct search* search);
    char* startSearch(struct fcb* fcb, struct search* search, BYTE u);
    void seekFile(struct fcb* fcb);
    int openFileFcb(struct fcb* fcb, BYTE u);

};

#endif // HOSTFILEBDOS_H_
