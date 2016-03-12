/// \file HostFileBdos.cpp
///
///  Virtual access to host filesystem via CP/Net-style I/O device.
///
///  \date Feb 19, 2016
///  \author Douglas Miller
///

#include "HostFileBdos.h"
#include "logger.h"

#include <sys/types.h>
#include <sys/stat.h>
#include <time.h>
#include <fcntl.h>
#include <unistd.h>
#include <dirent.h>
#include <string.h>
#include <fnmatch.h>

HostFileBdos::HostFileBdos(PropertyUtil::PropertyMapT& props,
                           std::vector<std::string> args, BYTE srvId):
    NetworkServer(),
    serverId(srvId),
    curDsk(-1),
    curUsr(0),
    curROVec(0),
    curLogVec(0)
{
    memset(&curSearch, 0, sizeof(curSearch));
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
}

int
HostFileBdos::checkRecvMsg(BYTE clientId, BYTE* msgbuf, int len)
{
    // For HostFileBdos, this indicates an error. The client did not
    // get the response we posted from sendMsg().
    return 0;
}

int
HostFileBdos::sendMsg(BYTE* msgbuf, int len)
{
    struct NetworkServer::ndos* hdr = (struct NetworkServer::ndos*) msgbuf;
    BYTE*  msg = msgbuf + sizeof(*hdr);

    if (bdosFunctions[hdr->mfunc] == NULL)
    {
        msg[0] = 0xff;
        msg[1] = 0x0c;
        return 2;
    }

    return bdosFunctions[hdr->mfunc](this, msg, len - sizeof(*hdr));
}

int
HostFileBdos::getDPB(BYTE* msgbuf, int len)
{
    memcpy(msgbuf, &curDpb, sizeof(curDpb));
    return sizeof(curDpb);
}
int
HostFileBdos::writeProt(BYTE* msgbuf, int len)
{
    curROVec |= (1 << (msgbuf[0] & 0x0f));
    msgbuf[0] = 0;
    return 1;
}
int
HostFileBdos::resetDrive(BYTE* msgbuf, int len)
{
    curROVec &= ~(1 << (msgbuf[0] & 0x0f));
    msgbuf[0] = 0;
    return 1;
}
int
HostFileBdos::getROVec(BYTE* msgbuf, int len)
{
    msgbuf[0] = (curROVec & 0x0ff);
    msgbuf[1] = ((curROVec >> 8) & 0x0ff);
    return 2;
}
int
HostFileBdos::getAllocVec(BYTE* msgbuf, int len)
{
    // TODO: is there anything here?
    memset(msgbuf, 0, 256);
    return 256;
}
int
HostFileBdos::getLoginVec(BYTE* msgbuf, int len)
{
    msgbuf[0] = (curLogVec & 0x0ff);
    msgbuf[1] = ((curLogVec >> 8) & 0x0ff);
    return 2;
}

int
HostFileBdos::selectDisk(BYTE* msgbuf, int len)
{
    BYTE d = msgbuf[0];
    msgbuf[0] = 0;

    if (curDsk == d)
    {
        return 1;
    }

    curDsk = d;
    char        buf[256];
    struct stat stb;
    cpmDrive(buf, d);
    int         rc = stat(buf, &stb);

    if (rc < 0 && errno == ENOENT)
    {
        debugss(ssHostFileBdos, INFO, "Mkdir %s\n", buf);
        rc = mkdir(buf, 0777);

        if (rc == 0)
        {
            stb.st_mode |= S_IFDIR;
        }
    }

    if (rc < 0 || !S_ISDIR(stb.st_mode))
    {
        debugss(ssHostFileBdos, INFO, "Seldisk error (%d) %s\n", errno, buf);
        curLogVec &= ~(1 << d);
        msgbuf[0]  = 0xff;
        return 1;
    }

    debugss(ssHostFileBdos, INFO, "Seldisk: %s\n", buf);
    curLogVec |= (1 << d);
    return 1;
}

int
HostFileBdos::openFile(BYTE* msgbuf, int len)
{
    BYTE        u   = msgbuf[0] & 0x1f;
    struct fcb* fcb = (struct fcb*) &msgbuf[1];
    msgbuf[0] = 0;
    // ignore password...
    int         rc  = openFileFcb(fcb, u);

    if (rc < 0)
    {
        msgbuf[0] = 0xff;
        return 1;
    }

    else
    {
        memcpy(&msgbuf[1], fcb, 36);
        return 31;
    }
}

int
HostFileBdos::closeFile(BYTE* msgbuf, int len)
{
    int         rc;
    BYTE        u   = msgbuf[0] & 0x1f;
    struct fcb* fcb = (struct fcb*) &msgbuf[1];
    msgbuf[0] = 0;

    if (!fcb->s1[1])
    {
        // special truncate for SUBMIT (CCP)
        off_t len = fcb->rc;
        len *= 128;
        rc   = ftruncate(fcb->fd, len);

        if (rc < 0)
        {
            debugss(ssHostFileBdos, ERROR, "ftruncate\n");
        }
    }

    rc      = close(fcb->fd);
    fcb->fd = -1;

    if (rc < 0)
    {
        msgbuf[0] = 0xff;
        return 1;
    }

    else
    {
        memcpy(&msgbuf[1], fcb, 36);
        return 37;
    }
}

int
HostFileBdos::searchNext(BYTE* msgbuf, int len)
{
    BYTE  u    = msgbuf[1] & 0x1f;
    msgbuf[0] = 0;
    char* name = doSearch(&curSearch);

    if (!name)
    {
        msgbuf[0] = 0xff;
        return 1;
    }

    copyOutSearch(&msgbuf[1], name);
    return 33;
}

int
HostFileBdos::searchFirst(BYTE* msgbuf, int len)
{
    BYTE        d   = msgbuf[0] & 0x0f;
    BYTE        u   = msgbuf[1] & 0x1f;
    struct fcb* fcb = (struct fcb*) &msgbuf[2];
    msgbuf[0] = 0;

    if (fcb->drv == '?')
    {
        debugss(ssHostFileBdos, ERROR, "Search with drv = '?'\n");
        fcb->drv = d;
    }

    char* f = startSearch(fcb, &curSearch, u);

    if (f == NULL)
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

        msgbuf[0] = 0xff;
        return 1;
    }

    curLogVec |= (1 << curSearch.drv);
    copyOutSearch(&msgbuf[1], f);
    return 33;
}

int
HostFileBdos::deleteFile(BYTE* msgbuf, int len)
{
    struct search era;
    BYTE          u   = msgbuf[0] & 0x1f;
    struct fcb*   fcb = (struct fcb*) &msgbuf[1];
    msgbuf[0] = 0;

    memset(&era, 0, sizeof(era));
    errno     = 0;
    char* name = startSearch(fcb, &era, u);
    int   rc   = 0;

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
HostFileBdos::readSeq(BYTE* msgbuf, int len)
{
    BYTE        u   = msgbuf[0] & 0x1f;
    struct fcb* fcb = (struct fcb*) &msgbuf[1];
    msgbuf[0] = 0;

    if (fcb->fd <= 0)
    {
        msgbuf[0] = 9;
        return 1;
    }

    if (fcb->cr != fcb->s1[0])
    {
        fcb->s1[0] = fcb->cr;
        off_t len = fcb->cr;
        len       *= 128;
        lseek(fcb->fd, len, SEEK_SET);
    }

    int rc = read(fcb->fd, &msgbuf[0x25], 128);

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

    // fill any partial "sector" with Ctrl-Z, in case it's text.
    memset(&msgbuf[0x25 + rc], 0x1a, 128 - rc);
    // detect media change?
    return 0xa5;
}

int
HostFileBdos::writeSeq(BYTE* msgbuf, int len)
{
    BYTE        u   = msgbuf[0] & 0x1f;
    struct fcb* fcb = (struct fcb*) &msgbuf[1];
    msgbuf[0] = 0;

    if (fcb->fd <= 0)
    {
        msgbuf[0] = 9;
        return 1;
    }

    int rc = write(fcb->fd, &msgbuf[0x25], 128);

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
    return 1;
}

int
HostFileBdos::createFile(BYTE* msgbuf, int len)
{
    char        buf[16];
    char        path[256];
    uint8_t     d;
    BYTE        u   = msgbuf[0] & 0x1f;
    struct fcb* fcb = (struct fcb*) &msgbuf[1];
    msgbuf[0] = 0;
    getFileName(buf, fcb);
    fcb->fd   = -1;
    d         = fcb->drv;

    if (!d)
    {
        d = curDsk;
    }

    else
    {
        --d;
    }

    cpmPath(path, d, u, buf);
    int flags = O_RDWR | O_CREAT | O_EXCL;
    int fd    = open(path, flags, 0666);

    if (fd < 0)
    {
        if (errno == ENOENT)
        {
            curLogVec &= ~(1 << d);
        }

        msgbuf[0] = 0xff;
        return 1;
    }

    curLogVec  |= (1 << d);
    fcb->ext    = 0;
    fcb->rc     = 0;
    fcb->fd     = fd;
    fcb->oflags = flags;
    fcb->s1[1]  = 0x80;
    memcpy(&msgbuf[1], fcb, 36);
    return 37;
}

int
HostFileBdos::renameFile(BYTE* msgbuf, int len)
{
    char        buf[32];
    char        old[256];
    char        newn[256];
    BYTE        d;
    BYTE        u   = msgbuf[0] & 0x1f;
    struct fcb* fcb = (struct fcb*) &msgbuf[1];
    msgbuf[0] = 0;
    getFileName(buf, fcb);
    d         = fcb->drv;

    if (!d)
    {
        d = curDsk;
    }

    else
    {
        --d;
    }

    cpmPath(old, d, u, buf);
    fcb = (struct fcb*) &msgbuf[17];
    getFileName(buf, fcb);
    cpmPath(newn, d, u, buf);
    int rc = rename(old, newn);

    if (rc < 0)
    {
        if (errno == ENOENT)
        {
            curLogVec &= ~(1 << d);
        }

        // todo: decode errno...
        msgbuf[0] = 0xff;
        return 1;
    }

    curLogVec |= (1 << d);
    return 1;
}

int
HostFileBdos::readRand(BYTE* msgbuf, int len)
{
    BYTE        u   = msgbuf[0] & 0x1f;
    struct fcb* fcb = (struct fcb*) &msgbuf[1];
    msgbuf[0] = 0;

    if (fcb->fd <= 0)
    {
        msgbuf[0] = 9;
        return 1;
    }

    seekFile(fcb);
    int rc = read(fcb->fd, &msgbuf[0x25], 128);
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

    memset(&msgbuf[0x25 + rc], 0x1a, 128 - rc);
    // detect media change?
    return 0xa5;
}

int
HostFileBdos::writeRand(BYTE* msgbuf, int len)
{
    BYTE        u   = msgbuf[0] & 0x1f;
    struct fcb* fcb = (struct fcb*) &msgbuf[1];
    msgbuf[0] = 0;

    if (fcb->fd <= 0)
    {
        msgbuf[0] = 9;
        return 1;
    }

    seekFile(fcb);
    int rc = write(fcb->fd, &msgbuf[0x25], 128);
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
HostFileBdos::setRandRec(BYTE* msgbuf, int len)
{
    BYTE        u   = msgbuf[0] & 0x1f;
    struct fcb* fcb = (struct fcb*) &msgbuf[1];
    msgbuf[0] = 0;

    if (fcb->fd <= 0)
    {
        msgbuf[0] = 9;
        return 1;
    }

    off_t r = lseek(fcb->fd, (off_t) 0, SEEK_CUR);
    r          = (r + 127) / 128;
    fcb->rr[0] = r & 0x0ff;
    r        >>= 8;
    fcb->rr[1] = r & 0x0ff;
    r        >>= 8;
    fcb->rr[2] = r & 0x0ff;
    return 37;
}

int
HostFileBdos::compFileSize(BYTE* msgbuf, int len)
{
    BYTE        u   = msgbuf[0] & 0x1f;
    struct fcb* fcb = (struct fcb*) &msgbuf[1];
    msgbuf[0] = 0;

    if (fcb->fd <= 0)
    {
        int rc = openFileFcb(fcb, u);

        if (rc < 0)
        {
            msgbuf[0] = 255;
            return 1;
        }

        fcb->fd = rc;
    }

    struct stat stb;

    int         rc = fstat(fcb->fd, &stb);

    if (rc < 0)
    {
        msgbuf[0] = 255;
        return 1;
    }

    off_t r = stb.st_size;
    r          = (r + 127) / 128;
    fcb->rr[0] = r & 0x0ff;
    r        >>= 8;
    fcb->rr[1] = r & 0x0ff;
    r        >>= 8;
    fcb->rr[2] = r & 0x0ff;
    return 37;
}

int
HostFileBdos::setFileAttrs(BYTE* msgbuf, int len)
{
    msgbuf[0] = 0;
    return 1;
}
int
HostFileBdos::accessDrive(BYTE* msgbuf, int len)
{
    msgbuf[0] = 0;
    return 1;
}
int
HostFileBdos::freeDrive(BYTE* msgbuf, int len)
{
    curDsk    = -1; // do this?
    msgbuf[0] = 0;
    return 1;
}
int
HostFileBdos::writeRandZF(BYTE* msgbuf, int len)
{
    // Linux zero-fills for us...
    return writeRand(msgbuf, len);
}
int
HostFileBdos::unlockRec(BYTE* msgbuf, int len)
{
    msgbuf[0] = 0;
    return 37;
}
int
HostFileBdos::login(BYTE* msgbuf, int len)
{
    msgbuf[0] = 0;
    return 1;
}
int
HostFileBdos::logoff(BYTE* msgbuf, int len)
{
    msgbuf[0] = 0;
    return 1;
}
int
HostFileBdos::setCompAttrs(BYTE* msgbuf, int len)
{
    msgbuf[0] = 0;
    return 1;
}
int
HostFileBdos::getServCfg(BYTE* msgbuf, int len)
{
    // TODO: implement this if needed...
    memset(msgbuf, 0, 17);
    return 17;
}
int
HostFileBdos::setDefPwd(BYTE* msgbuf, int len)
{
    msgbuf[0] = 0;
    return 1;
}
int HostFileBdos::getTime(BYTE *msgbuf, int len) {
    time_t now = time(NULL);
    struct tm tmv;
    localtime_r(&now, &tmv);
    // date is UTC, time is local, need to reconcile them...
    now += tmv.tm_gmtoff;
    int date = now / 86400 - 2922 + 1;
    msgbuf[0] = (date & 0x0ff);
    msgbuf[1] = ((date >> 8) & 0x0ff);
    msgbuf[2] = ((tmv.tm_hour / 10) << 4) | (tmv.tm_hour % 10);
    msgbuf[3] = ((tmv.tm_min / 10) << 4) | (tmv.tm_min % 10);
    msgbuf[4] = ((tmv.tm_sec / 10) << 4) | (tmv.tm_sec % 10);
    debugss(ssHostFileBdos, INFO, "getTime: %04x %02x %02x %02x\n",
               msgbuf[0] | (msgbuf[1] << 8), msgbuf[2], msgbuf[3], msgbuf[4]);
    return 5;
}

// '0' means not supported
int (*HostFileBdos::bdosFunctions[256])(HostFileBdos*, BYTE*, int) =
{
    0,            0,          0,            0,            0,          0,
    0,            0,          0,
    0,            0,          0,            0,            0, // 00-0D
    selectDisk,   openFile,   closeFile,    searchFirst,  searchNext,
    deleteFile,   readSeq,    writeSeq,     createFile,   renameFile,
    getLoginVec,
    0,            0, // 19-1A
    getAllocVec,  writeProt,  getROVec,     setFileAttrs, getDPB,
    0,               // 20
    readRand,     writeRand,  compFileSize, setRandRec,   resetDrive,
    accessDrive,  freeDrive,  writeRandZF,
    0,            0, // 29-2A
    unlockRec,
    0,            0, // 2C-2D
    0,            0,          0,            0,            0,          0,
    0,            0,          0,
    0,            0,          0,            0,            0,          0,
    0,            0,          0,               // 2E-3F
    login,        logoff,                      // not really implemented... but must not return error.
    0,            0,          0,            0, // 42-45
    setCompAttrs, getServCfg,
    0,            0,          0,            0,            0,          0,
    0,            0, // 48-4F
    0,            0,          0,            0,            0,          0,
    0,            0,          0,
    0,            0,          0,            0,            0,          0,
    0, // 50-5F
    0,            0,          0,            0,            0,          0,
    0,            0,          0,         // 60-68
    getTime,
    setDefPwd, // not really implemented...
    0
};

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

char*
HostFileBdos::cpmFindInit(struct search::find* find)
{
    find->dir = NULL;
}

char*
HostFileBdos::cpmFind(struct search::find* find, int drive, char* pattern)
{
    struct dirent* de;

    if (pattern)
    {
        if (find->dir)
        {
            closedir(find->dir);
        }

        strncpy(find->pat, pattern, sizeof(find->pat));
        find->pat[sizeof(find->pat) - 1] = '\0';
        find->dirlen                     = cpmDrive(find->path, drive);
        find->dir                        = opendir(find->path);

        if (!find->dir)
        {
            debugss(ssHostFileBdos, INFO, "no dir: %s\n", find->path);
            errno = ENXIO;
            return NULL;
        }
    }

    do
    {
        de = readdir(find->dir);

        if (!de)
        {
            errno = ENOENT;
            return NULL;
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
HostFileBdos::copyOutDir(BYTE* dma, char* name)
{
    struct fcb* fcb = (struct fcb*) dma;
    char*       t;
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

void
HostFileBdos::copyOutSearch(BYTE* buf, char* name)
{
    copyOutDir(buf, name);

    if (curSearch.ext == '?')
    {
        // return size of file... by some definition...
        // create fake info in FCB to fool caller into
        // thinking the file is of about the right size.
        struct fcb* fcb = (struct fcb*) buf;
        off_t       len = curSearch.size;
        len      = (len + 127) / 128; // num records
        fcb->ext = (len >> 7) & 0x1f; // max size < 512K
        fcb->rc  = len & 0x7f;
        len      = (len + curDpb.blm) >> curDpb.bsh;

        if (len > 16)
        {
            len = 16;
        }

        while (len)
        {
            --len;
            fcb->d0[len] = len + 1;
        }
    }
}

char*
HostFileBdos::commonSearch(struct search* search, char* pat)
{
    char* f = cpmFind(&search->find, search->drv, pat);

    if (!f)
    {
        return NULL;
    }

    if (search->ext == '?')
    {
        // return size of file... by some definition...
        struct stat stb;
        stat(cpmPathFound(&search->find), &stb);
        search->size = stb.st_size;
    }

    return f;
}

char*
HostFileBdos::doSearch(struct search* search)
{
    return commonSearch(search, NULL);
}

char*
HostFileBdos::startSearch(struct fcb* fcb, struct search* search, BYTE u)
{
    char pat[16];
    BYTE d = fcb->drv;

    if (d == '?')
    {
        d      = curDsk;
        pat[0] = '*';
        pat[1] = '\0';
    }

    else
    {
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

    search->drv  = d;
    search->usr  = u;
    search->ext  = fcb->ext;
    search->iter = 0;
    return commonSearch(search, pat);
}

void
HostFileBdos::seekFile(struct fcb* fcb)
{
    off_t r = fcb->rr[0] | (fcb->rr[0] << 8) | (fcb->rr[0] << 16);
    r         *= 128;
    (void) lseek(fcb->fd, r, SEEK_SET);
    fcb->s1[0] = fcb->cr;
}

int
HostFileBdos::openFileFcb(struct fcb* fcb, BYTE u)
{
    char    buf[32];
    char    path[256];
    uint8_t d;
    getFileName(buf, fcb);
    d = fcb->drv;

    if (!d)
    {
        d = curDsk;
    }

    else
    {
        --d;
    }

    cpmPath(path, d, u, buf);
    debugss(ssHostFileBdos, INFO, "Opening %s\n", path);
    int flags;

    if (access(path, W_OK) == 0)
    {
        flags = O_RDWR;
    }

    else
    {
        flags = O_RDONLY;
    }

    // NOTE: for this function, file must exist already.
    int fd = open(path, flags);

    if (fd < 0 && u)
    {
        debugss(ssHostFileBdos, INFO, "Fail to open %s (%d)\n", path, errno);
        cpmPath(path, d, 0, buf);
        fd = open(path, O_RDWR);
    }

    if (fd < 0)
    {
        debugss(ssHostFileBdos, INFO, "Fail to open %s (%d)\n", path, errno);
        // don't know exactly why... curLogVec &= ~(1 << d);
        return -1;
    }

    curLogVec  |= (1 << d); // drive must be valid
    fcb->ext    = 0;
    fcb->rc     = 0;
    fcb->fd     = fd;
    fcb->oflags = flags;
    struct stat stb;
    fstat(fd, &stb);
    off_t       len = stb.st_size;
    len        = (len + 127) / 128; // num records
    fcb->rc    = len & 0x7f;
    fcb->s1[0] = 0;
    fcb->s1[1] = 0x80; // flag if close needs to update...
    return 0;
}
