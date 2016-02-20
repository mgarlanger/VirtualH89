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

CPNetDevice::CPNetDevice(int base) :
    IODevice(base, 2),
    header((struct NetworkServer::ndos *)buffer),
    bufIx(0),
    msgLen(0),
    respLen(0)
{
}

CPNetDevice::~CPNetDevice()
{
}

void CPNetDevice::addServer(BYTE serverId, NetworkServer *server)
{
    servers[serverId] = server;
}

CPNetDevice *CPNetDevice::install_CPNetDevice(PropertyUtil::PropertyMapT& props)
{
    std::string s;

    s = props["cpnetdevice_port"];

    if (s.empty())
    {
        return NULL;
    }

    int port = strtoul(s.c_str(), NULL, 0);

    debugss(ssCPNetDevice, ERROR, "Creating CPNetDevice device at port %02x\n", port);
    CPNetDevice *cpnd = new CPNetDevice(port);

    // TODO: search properties for server definitions. For now, assume 00=HostFileBdos
    NetworkServer *nws = new HostFileBdos(props);
    cpnd->addServer(0, nws);

    return cpnd;
}

void CPNetDevice::reset()
{
    bufIx = 0;
    msgLen = 0;
    respLen = 0;
}

BYTE CPNetDevice::in(BYTE adr)
{
    BYTE off = getPortOffset(adr);
    BYTE val = 0x00;

    if (off == statusPortOffset)
    {
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
            header->mcode = 1; // CP/Net response
            header->mdid = clientId;
            header->msid = serverId;
            header->mfunc = 0; // TODO: save last func?
            header->msize = len - 1;
            respLen = sizeof(*header) + header->msize + 1;
            val |= sts_DataReady;
        }

        else if (len < 0)
        {
            debugss(ssCPNetDevice, ERROR, "Unexpected failure of checkRecvMsg()\n");
        }


        return val;
    }

    if (off == dataPortOffset)
    {
        // works for 'respLen == 0' (no data) case also.
        if (bufIx < respLen)
        {
            val = buffer[bufIx++];

            if (bufIx >= respLen)
            {
                debugss(ssCPNetDevice, INFO, "Response finished: %02x %02x %02x %02x %02x : %02x\n",
                        buffer[0], buffer[1], buffer[2], buffer[3], buffer[4], buffer[5]);
                respLen = 0;
                bufIx = 0;
            }
        }

        else
        {
            // error condition: reading bytes that don't exist.
            debugss(ssCPNetDevice, INFO, "Response overrun %d/%d\n", bufIx, respLen);
            respLen = -1;
            bufIx = 0;
        }

        return val;
    }

    debugss(ssCPNetDevice, ERROR, "Invalid port address %02x\n", adr);
}

void CPNetDevice::swapIds(struct NetworkServer::ndos *header)
{
    BYTE temp = header->mdid;
    header->mdid = header->msid;
    header->msid = temp;
}

void CPNetDevice::out(BYTE adr, BYTE val)
{
    BYTE off = getPortOffset(adr);

    if (off == statusPortOffset)
    {
        // reset / resync. other functions needed?
        bufIx = 0;
        msgLen = 0;
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
                    buffer[5] = 0xff;
                    header->msize = 1 - 1;
                }

                else
                {
                    clientId = header->msid; // TODO: get this during client init...
                    int len = sendMsg(buffer, msgLen);

                    if (len == 0)
                    {
                        // indicates async message sent - wait for recv later.
                        // but currently we don't support that.
                        bufIx = 0;
                        msgLen = 0;
                        return;
                    }

                    else if (len < 0)
                    {
                        debugss(ssCPNetDevice, ERROR, "Unexpected failure in sendMsg()\n");
                        // no response is coming, do something...
                        header->mcode |= 0x01;
                        swapIds(header);
                        buffer[5] = 0xff;
                        header->msize = 1 - 1;
                    }

                    header->msize = len - 1;
                    header->mcode |= 0x01;
                    swapIds(header);
                }

                bufIx = 0;
                msgLen = 0;
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

int CPNetDevice::checkRecvMsg(BYTE clientId, BYTE *msgbuf, int len)
{
    // TODO: interate over servers and call checkRecvMsg on each...
    return 0;
}

int CPNetDevice::sendMsg(BYTE *msgbuf, int len)
{
    struct NetworkServer::ndos *hdr = (struct NetworkServer::ndos *)msgbuf;
    NetworkServer *nws = servers[hdr->mdid];

    if (nws == NULL)
    {
        debugss(ssCPNetDevice, ERROR, "Attempting to send to non-existent server %02x\n", hdr->mdid);
        return -1; // reasonable?
    }

    debugss(ssCPNetDevice, INFO, "Message: %02x %02x %02x %02x %02x : %02x\n",
            msgbuf[0], msgbuf[1], msgbuf[2], msgbuf[3], msgbuf[4], msgbuf[5]);
    return nws->sendMsg(msgbuf, len);

}
