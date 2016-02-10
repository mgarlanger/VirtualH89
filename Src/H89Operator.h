/// \file H89Operator.h
///
/// \date Feb 9, 2016
/// \author Douglas Miller
///

#ifndef H89OPERATOR_H_
#define H89OPERATOR_H_

#include <assert.h>

#include "config.h"
#include "GenericDiskDrive.h"

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
    GenericDiskDrive *findDrive(std::string name);
    DiskController *findDiskCtrlr(std::string name);
    std::string cleanse(std::string resp);
};

#endif // H89OPERATOR_H_
