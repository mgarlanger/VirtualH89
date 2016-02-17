///
/// \name ClockUser.h
///
///  \date Aug 11, 2012
///  \author Mark Garlanger
///

#ifndef CLOCKUSER_H_
#define CLOCKUSER_H_



class ClockUser
{
  public:
    ClockUser();
    virtual ~ClockUser();

    virtual void notification(unsigned int cycleCount) = 0;

  private:

};



#endif // CLOCKUSER_H_
