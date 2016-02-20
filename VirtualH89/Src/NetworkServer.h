/// \file NetworkServer.h
///
///  Interface for network servers used with CPNetDevice.
///  First implementation was HostFileBdos.
///
///  \date Feb 19, 2016
///  \author Douglas Miller
///

#ifndef NETWORKSERVER_H_
#define NETWORKSERVER_H_

#include "config.h"
#include "h89Types.h"

class NetworkServer
{
  public:
    NetworkServer();
    virtual ~NetworkServer();

    virtual int checkRecvMsg(BYTE clientId, BYTE *msgbuf, int len) = 0;
    virtual int sendMsg(BYTE *msgbuf, int len) = 0;

    // This is the standard CP/Net message header.
    struct ndos
    {
        BYTE mcode; // cmd=00, response=01, for CP/Net
        BYTE mdid;
        BYTE msid;
        BYTE mfunc;
        BYTE msize; // size is msize+1 (1-256 bytes)
    };

  private:
};

#endif // NETWORKSERVER_H_
