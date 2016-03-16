/// \file CPNetDevice.cpp
///
///  Virtual access to host filesystem via CP/Net-style I/O device.
///
///  \date Feb 19, 2016
///  \author Douglas Miller
///

#include "CPNetDevice.h"
#include "logger.h"

#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <unistd.h>
#include <dirent.h>
#include <string.h>
#include <fnmatch.h>

///
/// User (CP/M) prootocol:
///
/// Init/reset:
///     OUT ctrlPort    ; RESET device
///     IN  dataPort    ; get clientId (only valid after RESET)
///     STA myClientId  ; save for CP/Net
///     ; other initialization...
///
/// Send:
///     MVI C,dataPort
///     LXI H,msgbuf
///     MVI B,5         ; header length
///     OUTIR           ; send header
///     LDA msgbuf+4    ; msg size (-1)
///     INR A
///     MOV B,A
///     OUTIR           ; send message body
///     IN  statusPort
///     ANI 02h         ; cmd overrun bit
///     RZ              ; message accepted
///     ; error case
///
/// Receive:
///     IN statusPort
///     ANI 01h         ; data ready
///     JZ  Receive
///     MVI C,dataPort
///     LXI H,msgbuf
///     MVI B,5         ; header length
///     INIR            ; get header
///     LDA msgbuf+4    ; msg size (-1)
///     INR A
///     MOV B,A
///     INIR            ; get message body
///     IN  statusPort
///     ANI 04h         ; rcv overrun bit
///     RZ              ; message OK
///     ; error case
///
/// For reference, standard CP/Net message header is:
/// +0  format code (00 = CP/Net send, 01 = response)
/// +1  dest node ID (server or this client, depending on direction)
/// +2  src node ID (this client or server, '')
/// +3  CP/Net, MP/M, CP/M BDOS function number
/// +4  msg size - 1 (00 = 1, FF = 256)
/// +5...   message body
///


CPNetDevice::CPNetDevice(int base, int cid):
    IODevice(base, 2),
    clientId(cid),
    header((struct NetworkServer::ndos*) buffer),
    bufIx(0),
    msgLen(0),
    respLen(0),
    initDev(false)
{
}

CPNetDevice::~CPNetDevice()
{
}

void
CPNetDevice::addServer(BYTE serverId, NetworkServer* server)
{
    servers[serverId] = server;
}

CPNetDevice*
CPNetDevice::install_CPNetDevice(PropertyUtil::PropertyMapT& props)
{
    std::string s;
    int         cid = 0xfe; // OK default if we have no network connections

    s = props["cpnetdevice_port"];

    if (s.empty())
    {
        return NULL;
    }

    int port = strtoul(s.c_str(), NULL, 0);

    s = props["cpnetdevice_clientid"];

    if (!s.empty())
    {
        int c = strtoul(s.c_str(), NULL, 0);

        if (c > 0x00 && c < 0xff)
        {
            cid = c;
        }

        else
        {
            debugss(ssCPNetDevice, ERROR, "Invalid CP/Net client ID \"%s\"\n", s.c_str());
        }
    }

    debugss(ssCPNetDevice, ERROR, "Creating CPNetDevice device at port %02x, client ID %02x\n",
            port, cid);
    CPNetDevice*                         cpnd = new CPNetDevice(port, cid);

    PropertyUtil::PropertyMapT::iterator it   = props.begin();

    for (; it != props.end(); ++it)
    {
        // property syntax: cpnetdevice_server## = ClassId [args...]
        // where '##' is serverId in hex.
        if (it->first.compare(0, 18, "cpnetdevice_server") == 0)
        {
            BYTE                     sid  = strtoul(it->first.substr(18).c_str(), NULL, 16);
            debugss(ssCPNetDevice, ERROR, "Server %02x: %s\n", sid, it->second.c_str());
            std::vector<std::string> args = PropertyUtil::splitArgs(it->second);

            if (args[0].compare("HostFileBdos") == 0)
            {
                NetworkServer* nws = new HostFileBdos(props, args, sid);
                cpnd->addServer(sid, nws);
            }

//          else if args[0].compare("Socket") == 0) {
//              NetworkServer *nws = new SocketServer(props, args, sid);
//              cpnd->addServer(sid, nws);
//          }
        }
    }

    return cpnd;
}

void
CPNetDevice::reset()
{
    initDev = false;
    bufIx   = 0;
    msgLen  = 0;
    respLen = 0;
}

BYTE
CPNetDevice::in(BYTE adr)
{
    BYTE off = getPortOffset(adr);
    BYTE val = 0x00;

    if (off == statusPortOffset)
    {
        initDev = false;

        if (respLen > 0)
        {
            val |= sts_DataReady;
        }

        if (respLen < 0)
        {
            val |= sts_RespUnderrun;
        }

        if (msgLen == BUFFER_OVERRUN)
        {
            val |= sts_CmdOverrun;
        }

        if (val != 0)
        {
            // any of these conditions mean the client must do work.
            debugss(ssCPNetDevice, INFO, "statusPort = %02x\n", val);
            return val;
        }

        // must be waiting for a response, check and see...
        // Should never get here for synchronous server handlers (e.g. CPNetDevice).
        int len = checkRecvMsg(clientId, buffer, sizeof(buffer));

        if (len > 0)
        {
            // This includes any failures or errors, must be returned as CP/Net errors.
            respLen = sizeof(*header) + header->msize + 1;
            val    |= sts_DataReady;
        }

        else if (len < 0)
        {
            debugss(ssCPNetDevice, ERROR, "Unexpected failure of checkRecvMsg()\n");
        }


        return val;
    }

    if (off == dataPortOffset)
    {
        if (initDev)
        {
            val     = clientId;
            initDev = false;
            return val;
        }

        initDev = false;

        // works for 'respLen == 0' (no data) case also.
        if (bufIx < respLen)
        {
            val = buffer[bufIx++];

            if (bufIx >= respLen)
            {
                debugss(ssCPNetDevice, INFO, "Response finished: %02x %02x %02x %02x %02x : %02x\n",
                        buffer[0], buffer[1], buffer[2], buffer[3], buffer[4], buffer[5]);
                respLen = 0;
                bufIx   = 0;
            }
        }

        else
        {
            // error condition: reading bytes that don't exist.
            debugss(ssCPNetDevice, INFO, "Response overrun %d/%d\n", bufIx, respLen);
            respLen = -1;
            bufIx   = 0;
        }

        return val;
    }

    debugss(ssCPNetDevice, ERROR, "Invalid port address %02x\n", adr);
    return val;
}

void
CPNetDevice::swapIds(struct NetworkServer::ndos* header)
{
    BYTE temp = header->mdid;
    header->mdid = header->msid;
    header->msid = temp;
}

void
CPNetDevice::out(BYTE adr, BYTE val)
{
    BYTE off = getPortOffset(adr);
    initDev = false;

    if (off == statusPortOffset)
    {
        // reset / resync. other functions needed?
        initDev = true; // send clientId on next input data port.
        bufIx   = 0;
        msgLen  = 0;
        respLen = 0;
        return;
    }

    if (off == dataPortOffset)
    {
        if (respLen > 0)
        {
            // response was not yet consumed.
            // force error?
            msgLen = BUFFER_OVERRUN;
            return;
        }

        if (bufIx < sizeof(struct NetworkServer::ndos))
        {
            buffer[bufIx++] = val;

            if (bufIx >= sizeof(struct NetworkServer::ndos))
            {
                msgLen = sizeof(struct NetworkServer::ndos) + header->msize + 1;
                debugss(ssCPNetDevice, INFO, "Setting msglen to %d\n", msgLen);
                debugss(ssCPNetDevice, INFO, "Recv hdr: %02x %02x %02x %02x %02x\n",
                        buffer[0], buffer[1], buffer[2], buffer[3], buffer[4]);
            }

            return;
        }

        if (bufIx < msgLen)
        {
            debugss(ssCPNetDevice, INFO, "Recv data[%d] %02x\n", bufIx, val);
            buffer[bufIx++] = val;

            if (bufIx >= msgLen)
            {
                // we have something to do...
                debugss(ssCPNetDevice, INFO, "Command: %02x %02x %02x %02x %02x : %02x\n",
                        buffer[0], buffer[1], buffer[2], buffer[3], buffer[4], buffer[5]);

                if (header->mcode != 0)
                {
                    // We only handle CP/Net messages, but not sure how to indicate
                    // an error to client for others...
                    debugss(ssCPNetDevice, ERROR, "Not a CP/Net message\n");
                    header->mcode |= 0x01;
                    swapIds(header);
                    buffer[5]      = 0xff;
                    header->msize  = 1 - 1;
                }

                else
                {
                    int len = sendMsg(buffer, msgLen);

                    if (len == 0)
                    {
                        // indicates async message sent - wait for recv later.
                        // but currently we don't support that.
                        bufIx  = 0;
                        msgLen = 0;
                        return;
                    }

                    else if (len < 0)
                    {
                        debugss(ssCPNetDevice, ERROR, "Unexpected failure in sendMsg()\n");
                        // no response is coming, do something...
                        header->mcode |= 0x01;
                        swapIds(header);
                        buffer[5]      = 0xff;
                        header->msize  = 1 - 1;
                    }

                    header->msize  = len - 1;
                    header->mcode |= 0x01;
                    swapIds(header);
                }

                bufIx   = 0;
                msgLen  = 0;
                respLen = sizeof(*header) + header->msize + 1;
                debugss(ssCPNetDevice, INFO, "Response: %02x %02x %02x %02x %02x : %02x\n",
                        buffer[0], buffer[1], buffer[2], buffer[3], buffer[4], buffer[5]);
            }

            // don't do anything, just wait for receiver to execute command?
            return;
        }

        // too many bytes - probably some lost/aborted message.
        // need to remember error condition for later.
        debugss(ssCPNetDevice, INFO, "Command buffer overrun %d/%d\n", bufIx, msgLen);
        msgLen = BUFFER_OVERRUN;
        return;
    }

    debugss(ssCPNetDevice, ERROR, "Invalid port address %02x\n", adr);
}

int
CPNetDevice::checkRecvMsg(BYTE clientId, BYTE* msgbuf, int len)
{
    // TODO: interate over servers and call checkRecvMsg on each...
    // server message should include these already? but clientId may not be
    // known to servers.
    //      header->mcode = 1; // CP/Net response
    //      header->mdid = clientId;
    //      header->msid = serverId;
    //      header->mfunc = 0; // TODO: save last func? get from
    //      header->msize = len - 1;
    return 0;
}

int
CPNetDevice::sendMsg(BYTE* msgbuf, int len)
{
    struct         NetworkServer::ndos* hdr = (struct NetworkServer::ndos*) msgbuf;
    NetworkServer* nws = servers[hdr->mdid];

    if (nws == NULL)
    {
        debugss(ssCPNetDevice, ERROR, "Attempting to send to non-existent server %02x\n",
                hdr->mdid);
        return -1; // reasonable?
    }

    debugss(ssCPNetDevice, INFO, "Message: %02x %02x %02x %02x %02x : %02x\n",
            msgbuf[0], msgbuf[1], msgbuf[2], msgbuf[3], msgbuf[4], msgbuf[5]);
    return nws->sendMsg(msgbuf, len);

}
