///
/// \name ParallelPortConnection.h
///
///
/// \date Aug 17, 2013
/// \author Mark Garlanger
///

#ifndef PARALLELPORTCONNECTION_H_
#define PARALLELPORTCONNECTION_H_

#include "h89Types.h"

class ParallelLink;

class ParallelPortConnection
{
  public:
    ParallelPortConnection();
    virtual ~ParallelPortConnection();

    //
    virtual void connectLink(ParallelLink* link);

    enum SignalType
    {
        /// Master Reset (Host to Disk System)
        st_MasterReset,
        /// Data Acknowledge (Host to Disk System)
        st_DTAK,
        /// Data Ready (Disk System to Host)
        st_DTR,
        /// Disk Drive Out (Disk System to Host)
        st_DDOUT,
        /// Busy (Disk System to Host)
        st_Busy,
        /// Error (Disk System to Host)
        st_Error
    };

    virtual void raiseSignal(SignalType sigType) = 0;
    virtual void lowerSignal(SignalType sigType) = 0;
    virtual void pulseSignal(SignalType sigType) = 0;

    // virtual void receiveData(BYTE val) = 0;

  protected:
    ParallelLink* link_m;

};

#endif // PARALLELPORTCONNECTION_H_
