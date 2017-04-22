/// \file HostFileBdos.cpp
///
///  Virtual access to host filesystem via CP/Net-style I/O device.
///
///  \date Feb 19, 2016
///  \author Douglas Miller
///


#include "HostFileBdos.h"
#ifdef __H89__
#include "logger.h"
#else

enum logLevel
{
    FATAL = 0,
    ERROR,
    WARNING,
    INFO,
    VERBOSE,
    ALL,
};
#ifndef CUR_LOG_LEVEL
#define CUR_LOG_LEVEL   ERROR
#endif
#ifndef LOG_FILE
#define LOG_FILE    stderr
#endif
#define debugss(subsys, level, args ...) \
    if (level <= CUR_LOG_LEVEL){         \
        fprintf(LOG_FILE, args);         \
    }

#endif // !H89

/// \cond
#include <sys/types.h>
#include <sys/stat.h>
#include <time.h>
#include <fcntl.h>
#include <unistd.h>
#include <dirent.h>
#include <string.h>
#include <fnmatch.h>
#include <stdlib.h>
/// \endcond


HostFileBdos::HostFileBdos(PropertyUtil::PropertyMapT& props,
                           std::vector<std::string> args, uint8_t srvId, uint8_t cltId):
    NetworkServer(),
    dir(nullptr),
    serverId(srvId),
    clientId(cltId),
    curDsk(-1),
    curUsr(0),
    curROVec(0),
    curLogVec(0)
{
    memset(&curSearch, 0, sizeof(curSearch));
    memset(&curDpb, 0, sizeof(curDpb));
    memset(openFiles, -1, sizeof(openFiles));
    memset(&sFcb, 0, sizeof(sFcb));
    sFcb.drv   = 0x21;
    curDpb.spt = 64; // some number
    curDpb.bsh = DEF_BLS_SH - 7;
    curDpb.blm = (1 << curDpb.bsh) - 1;
    curDpb.dsm = DEF_NBLOCKS - 1;
    curDpb.drm = 63; // some number
    if (curDpb.dsm >= 256)
    {
        curDpb.exm = (1 << (curDpb.bsh - 4)) - 1;
        phyExt     = 8 << curDpb.bsh;
    }
    else
    {
        curDpb.exm = (1 << (curDpb.bsh - 3)) - 1;
        phyExt     = 16 << curDpb.bsh;
    }
    curDpb.cks = 0; // perhaps should be non-zero, as
    // files can change without notice.
    // But clients shouldn't be handling that.

    // args[0] is our class name, like argv[0] in main().
    std::string s;
    if (args.size() > 1)
    {
        s = args[1];
    }
    else
    {
        s = props["hostfilebdos_root_dir"];
    }
    if (s.empty())
    {
        s  = getenv("HOME");
        s += "/HostFileBdos";
    }
    int rc = mkdir(s.c_str(), 0777);
    if (rc != 0 && errno != EEXIST)
    {
        debugss(ssHostFileBdos, ERROR, "Cannot create root directory %s\n", s.c_str());
        // what to do... all future accesses will likely return errors.
    }
    debugss(ssHostFileBdos, ERROR, "Creating HostFileBdos device with root dir %s\n", s.c_str());
    dir = strdup(s.c_str());
}

HostFileBdos::~HostFileBdos()
{
    int x;
    for (x = 0; x < DEF_NFILE; ++x)
    {
        if (openFiles[x] >= 0)
        {
            close(openFiles[x]);
            openFiles[x] = -1;
        }
    }
    if (curSearch.find.dir)
    {
        closedir(curSearch.find.dir);
        curSearch.find.dir = nullptr;
    }
    if (dir != nullptr)
    {
        free(dir);
        dir = nullptr;
    }
}

int
HostFileBdos::checkRecvMsg(uint8_t clientId, uint8_t* msgbuf, int len)
{
    // For HostFileBdos, this indicates an error. The client did not
    // get the response we posted from sendMsg().
    return 0;
}

int
HostFileBdos::sendMsg(uint8_t* msgbuf, int len)
{
    struct   NetworkServer::ndos* hdr = (struct NetworkServer::ndos*) msgbuf;
    uint8_t* msg = msgbuf + sizeof(*hdr);
    if (bdosFunctions[hdr->mfunc] == nullptr)
    {
        msg[0] = 255;
        msg[1] = 12;
        return 2;
    }
    return bdosFunctions[hdr->mfunc](this, msg, len - sizeof(*hdr));
}

int
HostFileBdos::getDPB(uint8_t* msgbuf, int len)
{
    memcpy(msgbuf, &curDpb, sizeof(curDpb));
    return sizeof(curDpb);
}
int
HostFileBdos::writeProt(uint8_t* msgbuf, int len)
{
    curROVec |= (1 << (msgbuf[0] & 0x0f));
    msgbuf[0] = 0;
    return 1;
}
int
HostFileBdos::resetDrive(uint8_t* msgbuf, int len)
{
    curROVec &= ~(1 << (msgbuf[0] & 0x0f));
    msgbuf[0] = 0;
    return 1;
}
int
HostFileBdos::getROVec(uint8_t* msgbuf, int len)
{
    msgbuf[0] = (curROVec & 0x0ff);
    msgbuf[1] = ((curROVec >> 8) & 0x0ff);
    return 2;
}
int
HostFileBdos::getAllocVec(uint8_t* msgbuf, int len)
{
    // TODO: is there anything here?
    memset(msgbuf, 0, 256);
    return 256;
}
int
HostFileBdos::getLoginVec(uint8_t* msgbuf, int len)
{
    msgbuf[0] = (curLogVec & 0x0ff);
    msgbuf[1] = ((curLogVec >> 8) & 0x0ff);
    return 2;
}

int
HostFileBdos::selectDisk(uint8_t* msgbuf, int len)
{
    uint8_t d = msgbuf[0];
    msgbuf[0] = 0;
    if (curDsk == d)
    {
        return 1;
    }
    curDsk = d;
    struct stat stb;
    cpmDrive(fileName, d);
    int         rc = stat(fileName, &stb);
    if (rc < 0 && errno == ENOENT)
    {
        debugss(ssHostFileBdos, INFO, "Mkdir %s\n", fileName);
        rc = mkdir(fileName, 0777);
        if (rc == 0)
        {
            stb.st_mode |= S_IFDIR;
        }
    }
    if (rc < 0 || !S_ISDIR(stb.st_mode))
    {
        debugss(ssHostFileBdos, INFO, "Seldisk error (%d) %s\n", errno, fileName);
        curLogVec &= ~(1 << d);
        msgbuf[0]  = 255;
        return 1;
    }
    debugss(ssHostFileBdos, INFO, "Seldisk: %s\n", fileName);
    curLogVec |= (1 << d);
    return 1;
}

int
HostFileBdos::openFile(uint8_t* msgbuf, int len)
{
    uint8_t     u   = msgbuf[0] & 0x1f;
    struct fcb* fcb = (struct fcb*) &msgbuf[1];
    msgbuf[0] = 0;
    // ignore password...
    int         rc  = openFileFcb(fcb, u);

    if (rc < 0)
    {
        msgbuf[0] = 255; // a.k.a. File Not Found
        return 1;
    }
    else
    {
        return 37;
    }
}

int
HostFileBdos::closeFile(uint8_t* msgbuf, int len)
{
    int         rc;
    // uint8_t u = msgbuf[0] & 0x1f;
    struct fcb* fcb = (struct fcb*) &msgbuf[1];
    int         fd  = getFileFcb(fcb);
    if (fd < 0)
    {
        msgbuf[0] = 255;
        return 1;
    }
    msgbuf[0] = 0;
    if (!fcb->s1[1])
    {
        // special truncate for SUBMIT (CCP)
        off_t len = fcb->rc;
        len *= 128;
        rc   = ftruncate(fd, len);
        if (rc < 0)
        {
            debugss(ssHostFileBdos, ERROR, "ftruncate\n");
        }
    }
    rc = closeFileFcb(fcb);
    if (rc < 0)
    {
        msgbuf[0] = 255;
        return 1;
    }
    else
    {
        return 37;
    }
}

int
HostFileBdos::searchNext(uint8_t* msgbuf, int len)
{
    // uint8_t u = msgbuf[1] & 0x1f;
    if (curSearch.full)
    {
        int dc = curSearch.iter & 0x03;
        if (curSearch.iter < curSearch.lastit)
        {
            ++curSearch.iter;
            msgbuf[0] = dc;
            return 1;
        }
        if (curSearch.iter < curSearch.endit)
        {
            msgbuf[0] = 255;
            return 1;
        }
        // force iter to next record
        curSearch.iter = (curSearch.iter + 0x03) & ~0x03;
    }
    const char* name = doSearch(&curSearch);
    if (!name)
    {
        msgbuf[0] = 255;
        return 1;
    }
    if (curSearch.full)
    {
        // curSearch.iter is 0, must be. We know at least one exists.
        msgbuf[0] = fullSearch(&msgbuf[1], &curSearch, name); // fill the dma buf, if possible.
        return 129;
    }
    else
    {
        msgbuf[0] = copyOutSearch(&msgbuf[1], name);
        return 33;
    }
}

// CP/M 3 DIR uses fcb->drv == '?' to do full listing,
// and older CP/M STAT uses fcb->ext == '?'.
// However, fcb->drv == '?' is documented in older CP/M
// so we cannot assume CP/M 3 based on that.
int
HostFileBdos::searchFirst(uint8_t* msgbuf, int len)
{
    // uint8_t d = msgbuf[0] & 0x0f; // should be curDsk...
    uint8_t     u   = msgbuf[1] & 0x1f;
    struct fcb* fcb = (struct fcb*) &msgbuf[2];
    if ((fcb->drv & 0x7f) == '?')
    {
        // This means return every dir entry...
        debugss(ssHostFileBdos, ERROR, "Search with drv = '?'\n");
        memset(&fcb->name[0], '?', 12);
    }
    const char* f = startSearch(fcb, &curSearch, u);
    if (f == nullptr)
    {
        if (errno == ENXIO)
        {
            // no drive present...
            curLogVec &= ~(1 << curSearch.drv);
        }
        else
        {
            curLogVec |= (1 << curSearch.drv);
        }
        msgbuf[0] = 255;
        return 1;
    }
    curLogVec |= (1 << curSearch.drv);
    if (curSearch.full)
    {
        // curSearch.iter is 0, must be. We know at least one exists.
        msgbuf[0] = fullSearch(&msgbuf[1], &curSearch, f); // fill the dma buf, if possible.
        return 129;
    }
    else
    {
        msgbuf[0] = copyOutSearch(&msgbuf[1], f);
        return 33;
    }
}

// TODO: if this is an open file, then close it?
int
HostFileBdos::deleteFile(uint8_t* msgbuf, int len)
{
    struct search era;
    uint8_t       u   = msgbuf[0] & 0x1f;
    msgbuf[0] = 0;
    struct fcb*   fcb = (struct fcb*) &msgbuf[1];
    if (fcb->drv == '?')
    {
        msgbuf[0] = 255;
        msgbuf[1] = 9;
        return 2;
    }
    if ((fcb->name[4] & 0x80) != 0) // check F5'
    {                               // no XFCBs in this implementation,
        // do not remove timestamps.
        return 1;
    }
    fcb->ext = 0; // in case it was '?'
    memset(&era, 0, sizeof(era));
    errno    = 0;
    const char* name = startSearch(fcb, &era, u);
    int         rc   = 0;
    while (name)
    {
        ++rc;
        unlink(cpmPathFound(&era.find));
        name = doSearch(&era);
    }
    if (rc == 0)
    {
        msgbuf[0] = 255;
        if (errno == ENXIO)
        {
            curLogVec &= ~(1 << era.drv);
        }
        return 1;
    }
    curLogVec |= (1 << era.drv);
    return 1;
}

int
HostFileBdos::readSeq(uint8_t* msgbuf, int len)
{
    // uint8_t u = msgbuf[0] & 0x1f;
    struct fcb* fcb = (struct fcb*) &msgbuf[1];
    msgbuf[0] = 0;
    int         fd  = getFileFcb(fcb);
    if (fd < 0)
    {
        msgbuf[0] = 9;
        return 1;
    }
    if ((fcb->ext == 0 && fcb->cr == 0) ||
        fcb->cr != fcb->s1[0])
    {
        off_t len = fcb->cr;
        len *= 128;
        lseek(fd, len, SEEK_SET);
    }
    ssize_t rc = read(fd, &msgbuf[37], 128);
    if (fcb->cr > 127)
    {
        fcb->cr   = 0;
        ++fcb->ext;
        fcb->ext &= 0x1f;
    }
    if (rc < 0)
    {
        msgbuf[0] = 255;
        return 1;
    }
    if (rc == 0)
    {
        msgbuf[0] = 1;
        return 1;
    }
    ++fcb->cr;
    fcb->s1[0] = fcb->cr;
    // fill any partial "sector" with Ctrl-Z, in case it's text.
    memset(&msgbuf[37 + rc], 0x1a, 128 - rc);
    // detect media change?
    return 165;
}

int
HostFileBdos::writeSeq(uint8_t* msgbuf, int len)
{
    // uint8_t u = msgbuf[0] & 0x1f;
    struct fcb* fcb = (struct fcb*) &msgbuf[1];
    msgbuf[0] = 0;
    int         fd  = getFileFcb(fcb);
    if (fd <= 0)
    {
        msgbuf[0] = 9;
        return 1;
    }
    ssize_t rc = write(fd, &msgbuf[37], 128);
    if (fcb->cr > 127)
    {
        fcb->cr   = 0;
        fcb->rc   = 0;
        ++fcb->ext;
        fcb->ext &= 0x1f;
    }
    ++fcb->cr;
    if (fcb->rc < fcb->cr)
    {
        fcb->rc = fcb->cr;
    }
    if (rc < 0)
    {
        msgbuf[0] = 255;
        return 1;
    }
    if (rc == 0)
    {
        msgbuf[0] = 1;
        return 1;
    }
    // fcb->s1[1] = 0; // don't need this here...
    // detect media change?
    return 37;
}

// File is open on successful return.
int
HostFileBdos::createFile(uint8_t* msgbuf, int len)
{
    uint8_t     u   = msgbuf[0] & 0x1f;
    struct fcb* fcb = (struct fcb*) &msgbuf[1];
    msgbuf[0] = 0;
    int         rc  = makeFileFcb(fcb, u);
    if (rc < 0)
    {
        msgbuf[0] = 255;
        return 1;
    }
    return 37;
}

int
HostFileBdos::renameFile(uint8_t* msgbuf, int len)
{
    char        newn[sizeof(pathName)];
    uint8_t     d;
    uint8_t     u   = msgbuf[0] & 0x1f;
    struct fcb* fcb = (struct fcb*) &msgbuf[1];
    msgbuf[0] = 0;
    getFileName(fileName, fcb);
    d         = fcb->drv;
    if (!d)
    {
        d = curDsk;
    }
    else
    {
        --d;
    }
    cpmPath(pathName, d, u, fileName);
    fcb = (struct fcb*) &msgbuf[17];
    getFileName(fileName, fcb);
    cpmPath(newn, d, u, fileName);
    int rc = rename(pathName, newn);
    if (rc < 0)
    {
        if (errno == ENOENT)
        {
            curLogVec &= ~(1 << d);
        }
        // todo: decode errno...
        msgbuf[0] = 255;
        return 1;
    }
    curLogVec |= (1 << d);
    return 1;
}

int
HostFileBdos::readRand(uint8_t* msgbuf, int len)
{
    // uint8_t u = msgbuf[0] & 0x1f;
    struct fcb* fcb = (struct fcb*) &msgbuf[1];
    msgbuf[0] = 0;
    int         fd  = getFileFcb(fcb);
    if (fd < 0)
    {
        msgbuf[0] = 9;
        return 1;
    }
    seekFile(fcb);
    ssize_t rc = read(fd, &msgbuf[37], 128);
    seekFile(fcb);
    if (rc < 0)
    {
        msgbuf[0] = 255;
        return 1;
    }
    if (rc == 0)
    {
        msgbuf[0] = 1;
        return 1;
    }
    memset(&msgbuf[37 + rc], 0x1a, 128 - rc);
    // detect media change?
    return 37 + 128;
}

int
HostFileBdos::writeRand(uint8_t* msgbuf, int len)
{
    // uint8_t u = msgbuf[0] & 0x1f;
    struct fcb* fcb = (struct fcb*) &msgbuf[1];
    msgbuf[0] = 0;
    int         fd  = getFileFcb(fcb);
    if (fd < 0)
    {
        msgbuf[0] = 9;
        return 1;
    }
    seekFile(fcb);
    ssize_t rc = write(fd, &msgbuf[37], 128);
    seekFile(fcb);
    if (rc < 0)
    {
        msgbuf[0] = 255;
        return 1;
    }
    if (rc == 0)
    {
        msgbuf[0] = 1;
        return 1;
    }
    // detect media change?
    return 37;
}

int
HostFileBdos::setRandRec(uint8_t* msgbuf, int len)
{
    // uint8_t u = msgbuf[0] & 0x1f;
    struct fcb* fcb = (struct fcb*) &msgbuf[1];
    msgbuf[0] = 0;
    int         fd  = getFileFcb(fcb);
    if (fd < 0)
    {
        msgbuf[0] = 9;
        return 1;
    }
    off_t r = lseek(fd, (off_t) 0, SEEK_CUR);
    if (r > 0x03ffff * 128)
    {
        r = 0x03ffff;
    }
    else
    {
        r = (r + 127) / 128;
    }
    fcb->rr[0] = r & 0x0ff;
    r        >>= 8;
    fcb->rr[1] = r & 0x0ff;
    r        >>= 8;
    fcb->rr[2] = r & 0x0ff;
    return 37;
}

int
HostFileBdos::compFileSize(uint8_t* msgbuf, int len)
{
    uint8_t     u   = msgbuf[0] & 0x1f;
    struct fcb* fcb = (struct fcb*) &msgbuf[1];
    msgbuf[0] = 0;
    int         fd  = getFileFcb(fcb);
    if (fd < 0)
    {
        int rc = openFileFcb(fcb, u);
        if (rc < 0)
        {
            msgbuf[0] = 255;
            return 1;
        }
        fd = getFileFcb(fcb);
    }
    struct stat stb;
    int         rc = fstat(fd, &stb);
    if (rc < 0)
    {
        msgbuf[0] = 255;
        return 1;
    }
    off_t r = stb.st_size;
    if (r > 0x03ffff * 128)
    {
        r = 0x03ffff;
    }
    else
    {
        r = (r + 127) / 128;
    }
    fcb->rr[0] = r & 0x0ff;
    r        >>= 8;
    fcb->rr[1] = r & 0x0ff;
    r        >>= 8;
    fcb->rr[2] = r & 0x003;
    return 37;
}

int
HostFileBdos::setFileAttrs(uint8_t* msgbuf, int len)
{
    msgbuf[0] = 0;
    return 1;
}
int
HostFileBdos::accessDrive(uint8_t* msgbuf, int len)
{
    msgbuf[0] = 0;
    return 1;
}
int
HostFileBdos::freeDrive(uint8_t* msgbuf, int len)
{
    curDsk    = -1; // do this?
    msgbuf[0] = 0;
    return 1;
}
int
HostFileBdos::writeRandZF(uint8_t* msgbuf, int len)
{
    // Linux zero-fills for us...
    return writeRand(msgbuf, len);
}
int
HostFileBdos::lockRec(uint8_t* msgbuf, int len)
{
    msgbuf[0] = 0;
    return 37;
}
int
HostFileBdos::unlockRec(uint8_t* msgbuf, int len)
{
    msgbuf[0] = 0;
    return 37;
}
int
HostFileBdos::getFreeSp(uint8_t* msgbuf, int len)
{
    msgbuf[0] = 0;
    msgbuf[1] = 0xff; // inifinte space available
    msgbuf[2] = 0xff;
    msgbuf[3] = 0xff;
    return 4;
}
int
HostFileBdos::flushBuf(uint8_t* msgbuf, int len)
{
    // nothing to do for us?
    msgbuf[0] = 0;
    return 1;
}
int
HostFileBdos::freeBlks(uint8_t* msgbuf, int len)
{
    // nothing to do for us?
    msgbuf[0] = 0;
    return 1;
}
int
HostFileBdos::truncFile(uint8_t* msgbuf, int len)
{
    int         rc;
    uint8_t     u   = msgbuf[0] & 0x1f;
    struct fcb* fcb = (struct fcb*) &msgbuf[1];
    msgbuf[0] = 0;
    int         fd  = getFileFcb(fcb);
    if (fd >= 0)
    {
        msgbuf[0] = 0xff;
        msgbuf[1] = 0;
        return 2;
    }
    uint8_t d = fcb->drv;
    if (!d)
    {
        d = curDsk;
    }
    else
    {
        --d;
    }
    getFileName(fileName, fcb);
    cpmPath(pathName, d, u, fileName);
    if (access(pathName, W_OK) != 0)
    {
        msgbuf[0] = 0xff;
        msgbuf[1] = errno == ENOENT ? 0x00 : 0x03;
        return 2;
    }
    struct stat stb;
    stat(pathName, &stb);
    off_t       r = fcb->rr[0] | (fcb->rr[1] << 8) | ((fcb->rr[2] & 0x03) << 16);
    if (r > stb.st_size)
    {
        msgbuf[0] = 0xff;
        msgbuf[1] = 0x00;
        return 2;
    }
    rc = truncate(pathName, r);
    if (rc < 0)
    {
        msgbuf[0] = 0xff;
        msgbuf[1] = 0xff;
        return 2;
    }
    // TODO: need to update ext,rc,cr? file is not open... as far as we know.
    return 37;
}
int
HostFileBdos::login(uint8_t* msgbuf, int len)
{
    msgbuf[0] = 0;
    return 1;
}
int
HostFileBdos::logoff(uint8_t* msgbuf, int len)
{
    msgbuf[0] = 0;
    return 1;
}
int
HostFileBdos::setCompAttrs(uint8_t* msgbuf, int len)
{
    msgbuf[0] = 0;
    return 1;
}
int
HostFileBdos::getServCfg(uint8_t* msgbuf, int len)
{
    // TODO: implement this if needed...
    memset(msgbuf, 0, 17);
    return 17;
}
int
HostFileBdos::setDefPwd(uint8_t* msgbuf, int len)
{
    msgbuf[0] = 0;
    return 1;
}
int
HostFileBdos::getDirLab(uint8_t* msgbuf, int len)
{
    // CP/M 3 DIR.COM uses full-search, not this, to
    // detect timestamps.
    msgbuf[0] = dirMode;
    return 1;
}
// Get timestamp for given file
int
HostFileBdos::readFStamps(uint8_t* msgbuf, int len)
{
    uint8_t     u   = msgbuf[0] & 0x1f;
    struct fcb* fcb = (struct fcb*) &msgbuf[1];
    msgbuf[0] = 0;
    uint8_t     d   = fcb->drv;
    if (!d)
    {
        d = curDsk;
    }
    else
    {
        --d;
    }
    getFileName(fileName, fcb);
    cpmPath(pathName, d, u, fileName);
    if (access(pathName, F_OK) != 0)
    {
        msgbuf[0] = 0xff;
        msgbuf[1] = 0x00;
        return 2;
    }
    struct stat stb;
    stat(pathName, &stb);
    fcb->ext = 0; // no passwords
    // order is important, overwrites seconds field (not present in CP/M 3 timestamps).
    unix2cpmdate(stb.st_atime, (struct cpmdate*) &fcb->d0[8]);
    unix2cpmdate(stb.st_mtime, (struct cpmdate*) &fcb->d0[12]);
    fcb->cr = 0;
    return 37;
}
int
HostFileBdos::getTime(uint8_t* msgbuf, int len)
{
    time_t          now = time(nullptr);
    struct cpmdate* cpm = (struct cpmdate*) &msgbuf[0];
    unix2cpmdate(now, cpm);
    debugss(ssHostFileBdos, INFO, "getTime: %04x %02x %02x %02x\n",
            msgbuf[0] | (msgbuf[1] << 8), msgbuf[2], msgbuf[3], msgbuf[4]);
    return 5;
}

// *INDENT-OFF*
// '0' means not supported (returns error if called)
int(*HostFileBdos::bdosFunctions[256])(HostFileBdos *, uint8_t *, int) = {
	0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0,		// (0-13)
	selectDisk, openFile, closeFile, searchFirst, searchNext, // 14-18
	deleteFile, readSeq, writeSeq, createFile, renameFile,	// 19-23
	getLoginVec,						// 24
	0, 0,							// (25-26)
	getAllocVec, writeProt, getROVec, setFileAttrs, getDPB,	// 27-31
	0,							// (32)
	readRand, writeRand, compFileSize, setRandRec, resetDrive, // 33-37
	accessDrive, freeDrive, writeRandZF,			// 38-40
	0,							// (41)
	lockRec, unlockRec,					// 42-43
	0, 0,							// (44-45)
	getFreeSp,						// 46
	0,							// (47)
	flushBuf,						// 48
	0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0,		// (49-63)
	login, logoff,	// 64, 65: not implemented but must not return error.
	0, 0, 0, 0,						// (66-69)
	setCompAttrs, getServCfg,				// 70, 71
	0, 0, 0, 0, 0, 0, 0, 0,					// (72-79)
	0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0,		// (80-95)
	0, 0,							// (96-97)
	freeBlks, truncFile,					// 98-99
	0, getDirLab, readFStamps, 0, 0,			// (100-104)
	getTime,	// 105: local convenience
	setDefPwd,	// 106: not really implemented...
	0
};
// *INDENT-ON*

int
HostFileBdos::cpmDrive(char* buf, int drive)
{
    drive &= 0x0f;
    return sprintf(buf, "%s/%c", dir, drive + 'a');
}

int
HostFileBdos::cpmFilename(char* buf, int user, char* file)
{
    int x = 0;
    user &= 0x1f;
    if (user)
    {
        x += sprintf(buf + x, "%d:", user);
    }
    x += sprintf(buf + x, "%s", file);
    return x;
}

int
HostFileBdos::cpmPath(char* buf, int drive, int user, char* file)
{
    int x = cpmDrive(buf, drive);
    buf[x++] = '/';
    x       += cpmFilename(buf + x, user, file);
    return x;
}

char*
HostFileBdos::cpmNameFound(struct search::find* find)
{
    return find->path + find->dirlen + 1;
}

char*
HostFileBdos::cpmPathFound(struct search::find* find)
{
    return find->path;
}

// Not used, as init is automatic on Search First.
void
HostFileBdos::cpmFindInit(struct search::find* find, int drive, char* pattern)
{
    if (find->dir)
    {
        closedir(find->dir);
    }
    strncpy(find->pat, pattern, sizeof(find->pat));
    find->pat[sizeof(find->pat) - 1] = '\0';
    find->dirlen                     = cpmDrive(find->path, drive);
    find->dir                        = opendir(find->path);
    // nullptr check done later...
    if (!find->dir)
    {
        debugss(ssHostFileBdos, INFO, "no dir: %s\n", find->path);
    }
}

const char*
HostFileBdos::cpmFind(struct search::find* find)
{
    struct dirent* de;
    if (!find->dir)
    {
        errno = ENXIO;
        return nullptr;
    }
    do
    {
        de = readdir(find->dir);
        if (!de)
        {
            errno = ENOENT;
            return nullptr;
        }
        if (de->d_name[0] == '.')
        {
            continue;
        }
        debugss(ssHostFileBdos, INFO, "looking at %s... %s\n", de->d_name, find->pat);
        // need to prevent "*.*" from matching all users!
        if (fnmatch(find->pat, de->d_name, 0) == 0)
        {
            break;
        }
    }
    while (1);
    sprintf(find->path + find->dirlen, "/%s", de->d_name);
    return cpmNameFound(find);
}

void
HostFileBdos::getFileName(char* dst, struct fcb* fcb)
{
    int x;
    for (x = 0; x < 8 && (fcb->name[x] & 0x7f) != ' '; ++x)
    {
        *dst++ = tolower(fcb->name[x] & 0x7f);
    }
    for (x = 8; x < 11 && (fcb->name[x] & 0x7f) != ' '; ++x)
    {
        if (x == 8)
        {
            *dst++ = '.';
        }
        *dst++ = tolower(fcb->name[x] & 0x7f);
    }
    *dst++ = '\0';
}

void
HostFileBdos::getAmbFileName(char* dst, struct fcb* fcb, uint8_t usr)
{
    int   x;
    char  buf[16];
    char* s    = buf;
    int   sawQ = 0;
    for (x = 0; x < 8 && (fcb->name[x] & 0x7f) != ' '; ++x)
    {
        if ((fcb->name[x] & 0x7f) == '?')
        {
            *s++ = '*';
            sawQ = 1;
            break;
        }
        *s++ = tolower(fcb->name[x] & 0x7f);
    }
    for (x = 8; x < 11 && (fcb->name[x] & 0x7f) != ' '; ++x)
    {
        if (x == 8)
        {
            if ((fcb->name[x] & 0x7f) == '?')
            {
                // special case, need "*.*" to be just "*"
                if (sawQ)
                {
                    break;
                }
            }
            *s++ = '.';
        }
        if ((fcb->name[x] & 0x7f) == '?')
        {
            *s++ = '*';
            break;
        }
        *s++ = tolower(fcb->name[x] & 0x7f);
    }
    *s++ = '\0';
    cpmFilename(dst, usr, buf);
}

void
HostFileBdos::copyOutDir(uint8_t* dma, const char* name)
{
    struct fcb* fcb = (struct fcb*) dma;
    const char* t;
    t = strchr(name, ':');
    uint8_t     u;
    if (t)
    {
        u = atoi(name);
        ++t;
    }
    else
    {
        u = 0;
        t = name;
    }
    fcb->drv = u; // user code , not drive
    int x = 0;
    while (*t && *t != '.' && x < 8)
    {
        fcb->name[x++] = toupper(*t++);
    }
    while (x < 8)
    {
        fcb->name[x++] = ' ';
    }
    while (*t && *t != '.')
    {
        ++t;
    }
    if (*t == '.')
    {
        ++t;
    }
    while (*t && *t != '.' && x < 11)
    {
        fcb->name[x++] = toupper(*t++);
    }
    while (x < 11)
    {
        fcb->name[x++] = ' ';
    }
    fcb->ext   = 0;
    fcb->s1[0] = 0;
    fcb->s1[1] = 0;
    fcb->rc    = 0;
    memset(fcb->d0, 0, sizeof(fcb->d0));
}

int
HostFileBdos::copyOutSearch(uint8_t* buf, const char* name)
{
    if (*name == ' ') // DIR LABEL
    {
        struct dirl* dirLab = (struct dirl*) buf;
        sprintf((char*) &dirLab->name[0], "SERVER%02x%c  ",
                serverId, curSearch.drv + 'A');
        dirLab->ext = dirMode;
        dirLab->drv = 0x20;
        // nothing meaningful for timestamps?
        return curSearch.iter++ & 0x03; // should be "0" (iter == 0)
    }
    else if (*name == '!')              // SFCB (already populated)
    {                                   // this should never happen...
        debugss(ssHostFileBdos, ERROR, "Copy Out SFCB in wrong place\n");
        memcpy(buf, &sFcb, 32);
        return curSearch.iter++ & 0x03; // should be "3"
    }
    copyOutDir(buf, name);
    struct fcb* fcb = (struct fcb*) buf;
    // curSearch.size is only valid if curSearch.ext == '?'.
    if (curSearch.ext == '?')
    {
        // create fake info in FCB to fool caller into
        // thinking the file is of about the right size.
        // This does not account for sparse files.
        off_t len = curSearch.size; // len in bytes
        if (len < phyExt * 128)
        {
            curSearch.size = 0;
        }
        else
        {
            curSearch.size -= phyExt * 128;
        }
        len = (len + 127) / 128; // len in records
        // STAT only cares about (ext & exm), so actually ext can wrap.
        // But, (ext & exm) must reflect the number of
        // logical extents in this dirent.
        int nb = 16, x;
        if (len <= phyExt)
        {
            nb       = (int) ((len + curDpb.blm) >> curDpb.bsh);
            fcb->ext = len / 128; // num logical extents
            fcb->rc  = len & 0x7f;
            if (len > 0 && fcb->rc == 0)
            {
                fcb->rc = 128;
            }
        }
        else
        {
            fcb->ext = curDpb.exm; // should have advancing num in hi bits
            fcb->rc  = 128;
        }
        for (x = 0; x < nb; ++x)
        {
            // just need a non-zero value...
            fcb->d0[x] = 1;
        }
    }
    // Generally, no one (STAT) cares if this is always 0.
    // CP/Net will place this in a DMA buffer for us.
    return curSearch.iter++ & 0x03;
}

const char*
HostFileBdos::commonSearch(struct search* search)
{
    if (search->full && search->cpm3)
    {
        if (search->iter == 0) // DIR LABEL
        {
            return " ";
        }
        else if ((search->iter & 0x03) == 0x03) // SFCB
        {
            return "!";
        }
    }
    if (search->ext == '?' && search->size > 0)
    {
        // copyOutSearch() should ensure we don't keep landing here,
        // but caller MUST invoke copyOutSearch() each time.
        return cpmNameFound(&search->find);
    }
    const char* f = cpmFind(&search->find);
    if (!f)
    {
        return nullptr;
    }
    if (search->ext == '?') // this includes search->full
    {                       // return size of file... by some definition...
        struct stat stb;
        stat(cpmPathFound(&search->find), &stb);
        search->size = stb.st_size;
        if (search->full && search->cpm3)
        {
            // SFCB timestamps are only used if the
            // matching FCB is for extent 0. However,
            // we just populate it anyway, especially since
            // extent info is not computed until copyOut.
            int x = search->iter & 0x03; // 0, 1, 2 only
            // order is imperative, overwrite seconds field
            unix2cpmdate(stb.st_atime,
                         (struct cpmdate*) &sFcb.fcbs[x].atim);
            unix2cpmdate(stb.st_mtime,
                         (struct cpmdate*) &sFcb.fcbs[x].utim);
            sFcb.fcbs[x].pwmode = 0; // clear seconds overrun
        }
    }
    return f;
}

int
HostFileBdos::fullSearch(uint8_t* dirbuf, struct search* search, const char* ff)
{
    int ix       = 0; // by definition, we are starting with DIRENT 0.
    int saveIter = search->iter;
    search->endit = search->iter + curSearch.maxdc;
    do
    {
        (void) copyOutSearch(dirbuf, ff);
        dirbuf += 32;
        ++ix;
        ff      = doSearch(search);
    }
    while (ff != nullptr && ix < search->maxdc); // never includes SFCB
    search->lastit = search->iter;               // if >= saveIter+maxdc then no error
    search->iter   = saveIter;
    while (ix < 4)
    {
        if (ix == search->maxdc) // can only happen for SFCB
        {
            memcpy(dirbuf, &sFcb, 32);
        }
        else
        {
            dirbuf[0] = 0xe5;
        }
        dirbuf += 32;
        ++ix;
    }
    return search->iter++ & 0x03;
}

const char*
HostFileBdos::doSearch(struct search* search)
{
    return commonSearch(search);
}

const char*
HostFileBdos::startSearch(struct fcb* fcb, struct search* search, uint8_t u)
{
    char    pat[16];
    uint8_t d = fcb->drv & 0x7f;
    search->iter = 0;
    if (d == '?')
    {
        search->full  = 1;
        search->cpm3  = ((fcb->drv & 0x80) != 0); // CP/M 3 sets hi bit
        search->ext   = '?';
        search->maxdc = (search->cpm3 ? 3 : 4);
        d             = curDsk;
        pat[0]        = '*';
        pat[1]        = '\0';
    }
    else
    {
        search->full = 0;
        search->cpm3 = 0;        // don't care if !full
        search->ext  = fcb->ext; // might still be '?' (e.g. CP/M 2 STAT.COM)
        if (!d)
        {
            d = curDsk;
        }
        else
        {
            --d;
        }
        getAmbFileName(pat, fcb, u);
    }
    search->drv = d;
    search->usr = u;
    cpmFindInit(&search->find, search->drv, pat);
    return commonSearch(search);
}

void
HostFileBdos::seekFile(struct fcb* fcb)
{
    int fd = getFileFcb(fcb);
    if (fd < 0)
    {
        return;
    }
    off_t r = fcb->rr[0] | (fcb->rr[1] << 8) | ((fcb->rr[2] & 0x03) << 16);
    r         *= 128;
    (void) lseek(fd, r, SEEK_SET);
    fcb->s1[0] = fcb->cr;
}

int
HostFileBdos::newFileFcb()
{
    int x;
    for (x = 0; x < DEF_NFILE; ++x)
    {
        if (openFiles[x] < 0)
        {
            break;
        }
    }
    if (x >= DEF_NFILE)
    {
        return -1;
    }
    return x;
}

int
HostFileBdos::locFileFcb(struct fcb* fcb)
{
    if ((fcb->fd ^ fcb->fd_) != 0xffff)
    {
        return -1;
    }
    int x = fcb->fd;
    if (x < 0 || x >= DEF_NFILE)
    {
        fcb->fd = fcb->fd_ = 0;
        return -1;
    }
    return x;
}

void
HostFileBdos::putFileFcb(struct fcb* fcb, int ix, int fd)
{
    openFiles[ix] = fd;
    if (fd < 0)
    {
        fcb->fd = fcb->fd_ = 0;
    }
    else
    {
        fcb->fd  = ix;
        fcb->fd_ = ~ix;
    }
    // Always put file ID in FCB, the
    // NDOS decides whether to copy it out.
    // In the case of Close, the File Id returned
    // is no longer valid.
    fcb->rr[0] = ix & 0x0ff;
    fcb->rr[1] = (ix >> 8) & 0x0ff;
}

// Returns Unix file descriptor, not CP/M File ID...
int
HostFileBdos::getFileFcb(struct fcb* fcb)
{
    int x = locFileFcb(fcb);
    if (x < 0)
    {
        return -1;
    }
    return openFiles[x];
}

// Returns CP/M File Id, or -1 on error.
int
HostFileBdos::closeFileFcb(struct fcb* fcb)
{
    int x = locFileFcb(fcb);
    if (x < 0)
    {
        return -1;
    }
    int fd = openFiles[x];
    putFileFcb(fcb, x, -1);
    int rc = close(fd);
    return rc < 0 ? -1 : x;
}

// Returns CP/M File Id, or -1 on error.
int
HostFileBdos::openFileFcb(struct fcb* fcb, uint8_t u)
{
    uint8_t d;
    int     x = newFileFcb();
    if (x < 0)
    {
        return -1;
    }
    getFileName(fileName, fcb);
    d = fcb->drv;
    if (!d)
    {
        d = curDsk;
    }
    else
    {
        --d;
    }
    cpmPath(pathName, d, u, fileName);
    debugss(ssHostFileBdos, INFO, "Opening %s\n", pathName);
    int flags;
    if (access(pathName, F_OK) < 0 && u)
    {
        cpmPath(pathName, d, 0, fileName);
    }
    if (access(pathName, W_OK) == 0)
    {
        flags = O_RDWR;
    }
    else
    {
        flags = O_RDONLY;
    }
    // NOTE: for this function, file must exist already.
    int fd = open(pathName, flags);
    if (fd < 0)
    {
        debugss(ssHostFileBdos, INFO, "Fail to open %s (%d)\n", pathName, errno);
        // don't know exactly why... curLogVec &= ~(1 << d);
        return -1;
    }
    putFileFcb(fcb, x, fd);
    curLogVec  |= (1 << d); // drive must be valid
    fcb->ext    = 0;
    fcb->oflags = flags;
    struct stat stb;
    fstat(fd, &stb);
    off_t       len = stb.st_size;
    len        = (len + 127) / 128; // num records
    fcb->rc    = len > 127 ? 128 : (len & 0x7f);
    fcb->s1[0] = 0;
    fcb->s1[1] = 0x80; // flag if close needs to update...
    return x;
}

// Returns CP/M File Id, or -1 on error.
int
HostFileBdos::makeFileFcb(struct fcb* fcb, uint8_t u)
{
    uint8_t d;

    int     x = newFileFcb();
    if (x < 0)
    {
        return -1;
    }
    getFileName(fileName, fcb);
    d = fcb->drv;
    if (!d)
    {
        d = curDsk;
    }
    else
    {
        --d;
    }
    cpmPath(pathName, d, u, fileName);
    int flags = O_RDWR | O_CREAT | O_EXCL;
    int fd    = open(pathName, flags, 0666);
    if (fd < 0)
    {
        if (errno == ENOENT)
        {
            curLogVec &= ~(1 << d);
        }
        return -1;
    }
    putFileFcb(fcb, x, fd);
    curLogVec  |= (1 << d);
    fcb->ext    = 0;
    fcb->rc     = 0;
    fcb->oflags = flags;
    fcb->s1[1]  = 0x80; // flag to update on close
    return x;
}

void
HostFileBdos::unix2cpmdate(time_t unx, struct cpmdate* cpm)
{
    struct tm tmv;
    localtime_r(&unx, &tmv);
    // date is UTC, time is local, need to reconcile them...
    unx      += tmv.tm_gmtoff;
    cpm->date = unx / 86400 - 2922 + 1;
    // the rest are BCD fields...
    cpm->hour = ((tmv.tm_hour / 10) << 4) | (tmv.tm_hour % 10);
    cpm->min  = ((tmv.tm_min / 10) << 4) | (tmv.tm_min % 10);
    cpm->sec  = ((tmv.tm_sec / 10) << 4) | (tmv.tm_sec % 10);
}
