///
/// \name ParallelLink.h
///
///
/// \date Aug 12, 2013
/// \author Mark Garlanger
///

#ifndef PARALLELLINK_H_
#define PARALLELLINK_H_

#include "h89Types.h"

class ParallelPortConnection;

/// Parallel Interface between the H/Z-47 Disk Drive System to the H8 or H89 computer
///
/// I/O Transaction Sequence
///
/// All I/O transactions between the computer and the disk system operates as follows:
///
///   a. The first byte transferred to the subsystem while it is in a system ready
///      state (that is busy false and DTR true) is interpreted as a command. The disk
///      system will set busy true and DTR false upon receipt of a command (including
///      invalid commands).
///
///   b. The disk system will determine the direction of data transfer using the DDOut
///      line. DDOut true indicates data flow to the computer; DDOut false indicates data
///      flow from the computer.
///
///   c. All data transfers are requested by the disk system by setting DTR true.
///
///   d. All data transfers must be acknowledged by the computer with a DTAK in response
///      to a DTR from the disk system. NOTE: Data input or output, to or from the computer,
///      precede a DTAK. Data output from the H8 must remain valid until DTR goes low.
///
///   e. The error flag is valid for the preceding operation (that is, when the disk system
///      completes an operation and sets BUSY false and DTR true).
///
///   f. MRST (Master ReSeT) will unconditionally abort any command and set the disk system
///      to the system ready state. The error flag will not be valid.
///
///   g. When a command has completed, the disk system will return to the system ready state.
///
/// Depending on the command presented by the computer, the disk system will request
/// additional parameters (unit number, track number, sector number, etc.) in the sequence
/// outlined in steps c and d above.
///
class ParallelLink
{
public:
    ParallelLink();
    virtual ~ParallelLink();

    virtual void sendHostData(BYTE val);
    virtual void sendDriveData(BYTE val);

    virtual void readDataBusByHost(BYTE &val);
    virtual void readDataBusByDrive(BYTE &val);

    virtual void setBusy(bool val);
    virtual bool readBusy();

    virtual void setDTR(bool val);
    virtual bool readDTR();

    virtual void masterReset();

    virtual void setDDOut(bool val);
    virtual bool readDDOut();

    virtual void setDTAK(bool val);
    virtual bool readDTAK();

    virtual void setError(bool val);
    virtual bool readError();

    virtual void registerDevice(ParallelPortConnection *device);
    virtual void registerHost(ParallelPortConnection *host);

protected:
    BYTE data_m;

    bool dataFromHost_m;
    bool dataFromDrive_m;

    /// Busy output. A true indicates that the disk system has received a command and is
    /// in the process of executing that command (including illegal commands). A low
    /// indicates that the disk sytem is idle and will accept a command.
    bool busy_m;

    /// Data Transfer Request output. A true indicates that the the disk system requires
    /// data on the data bas for an input (DDOUT false) or has placed data on the bus for an
    /// output (DDOUT true). DTR will become false after the data bus is invalid.
    bool DTR_m;

    /// Master ReSeT input. A true will cause the system to stop any operation in process,
    /// clear error conditions, raise all master and slave heads, and reset the disk system
    /// to an idle state.
    bool MRST_m;

    /// Data Direction OUT to H8. The data direction signal line is controlled by the disk
    /// system to indicate the direction that data is being transmitted on the bidirectional
    /// data bus D0 through D7. A false signal indicates a data transfer into the disk system.
    /// A true indicates a data transfer to the H8.
    bool DDOut_m;

    /// Data Transfer AcKnowledge. A true from the H8 acknowledges that data has been place
    /// on or taken from the data bus, depending on the level of DDOut in response to DTR.
    bool DTAK_m;

    /// ERROR output. A true indicates that one of the following error conditions has
    /// occurred:
    ///   (1) Record not found
    ///   (2) CRC error
    ///   (3) Lost data
    ///   (4) Illegal command
    ///   (5) Excess bad tracks
    ///   (6) Drive not ready when accessed
    ///   (7) Attempting to write on a write protected diskette
    ///   (8) Sector with deleted data mark encountered.
    /// The specific error condition(s) can be found by a read status command.
    bool error_m;


    // pointer to the host computer
    ParallelPortConnection *host_m;

    // pointer to the drive subsystem
    ParallelPortConnection *device_m;

};

#endif // PARALLELLINK_H_
