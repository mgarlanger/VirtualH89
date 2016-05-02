/// \file CPNetDevice.h
///
///  Virtual access to host filesystem via CP/Net-style I/O device.
///
///  \date Feb 19, 2016
///  \author Douglas Miller
///

#ifndef CPNETDEVICE_H_
#define CPNETDEVICE_H_

#include "IODevice.h"
#include "NetworkServer.h"
#include "propertyutil.h"

#include <dirent.h>
#include <string>



class CPNetDevice: public IODevice
{
  public:
    CPNetDevice(int base, int clientId);
    virtual ~CPNetDevice();
    static CPNetDevice* install_CPNetDevice(PropertyUtil::PropertyMapT& props);

    virtual BYTE in(BYTE adr);
    virtual void out(BYTE adr, BYTE val);
    virtual void reset();
  private:
    std::map<BYTE, NetworkServer*> servers;
    void addServer(BYTE serverId, NetworkServer* server);
    void swapIds(struct NetworkServer::ndos* header);
    int sendMsg(BYTE* msgbuf, int len);
    int checkRecvMsg(BYTE clientId, BYTE* msgbuf, int len);

    static const int  ndosLen = sizeof(struct NetworkServer::ndos);
    BYTE              clientId;
    const char*       dir;
    BYTE              buffer[256 + ndosLen];
    struct            NetworkServer::ndos* header;
    int               bufIx;
    int               msgLen;
    int               respLen;
    bool              initDev;

    static const BYTE serverId         = 0;

    static const int  BUFFER_OVERRUN   = 512; // any value larger than buffer[]

    static const int  dataPortOffset   = 0;
    static const int  statusPortOffset = 1;

    static const BYTE sts_DataReady    = 0x01;
    static const BYTE sts_CmdOverrun   = 0x02;
    static const BYTE sts_RespUnderrun = 0x04;
    static const BYTE sts_Error        = 0x08;
};

#endif // CPNETDEVICE_H_
