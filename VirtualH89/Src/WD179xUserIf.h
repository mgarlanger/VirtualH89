///
///  \name WD179xUserIf.h
///
///  \date April 9, 2016
///  \author Mark Garlanger
///

#ifndef WD179X_USER_IF_H_
#define WD179X_USER_IF_H_

class GenericFloppyDrive;

class WD179xUserIf
{
  public:
    WD179xUserIf();
    virtual ~WD179xUserIf();

    virtual void raiseIntrq()                     = 0;
    virtual void raiseDrq()                       = 0;
    virtual void lowerIntrq()                     = 0;
    virtual void lowerDrq()                       = 0;
    virtual void loadHead(bool load);
    virtual bool doubleDensity()                  = 0;
    virtual GenericFloppyDrive* getCurrentDrive() = 0;
    virtual int getClockPeriod()                  = 0;
    virtual bool readReady()                      = 0;

};

#endif // WD179X_USER_IF_H_
