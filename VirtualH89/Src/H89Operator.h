/// \file H89Operator.h
///
/// The VirtualH89-side implementation of the H89 Operator's interface, for
/// standard functions such as RESET and diskette muont/unmount, but also
/// providing virtual features like debugging.
///
/// \date Feb 9, 2016
/// \author Douglas Miller
///

#ifndef H89OPERATOR_H_
#define H89OPERATOR_H_

/// \cond
#include <assert.h>
#include <string>
/// \endcond


class GenericDiskDrive;
class DiskController;


/// \brief H89Operator
///
///
class H89Operator
{
  public:
    H89Operator();
    virtual ~H89Operator();
    std::string handleCommand(std::string cmd);

  private:
    std::string executeCommand(std::string cmd);
    GenericDiskDrive* findDrive(std::string name);
    DiskController* findDiskCtrlr(std::string name);
    std::string cleanse(std::string resp);
};

#endif // H89OPERATOR_H_
