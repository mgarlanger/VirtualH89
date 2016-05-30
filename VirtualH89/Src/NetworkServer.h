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

/// \cond
#include <stdint.h>
/// \endcond

class NetworkServer
{
  public:
    NetworkServer();
    virtual ~NetworkServer();

    virtual int checkRecvMsg(uint8_t clientId, uint8_t* msgbuf, int len) = 0;
    virtual int sendMsg(uint8_t* msgbuf, int len)                        = 0;

    // This is the standard CP/Net message header.
    struct ndos
    {
        uint8_t mcode; // cmd=00, response=01, for CP/Net
        uint8_t mdid;
        uint8_t msid;
        uint8_t mfunc;
        uint8_t msize; // size is msize+1 (1-256 bytes)
    };
  private:
};

#endif // NETWORKSERVER_H_
