/// \file Memory.h
///
/// \brief Base class for all types of memory (RAM or ROM).
///
/// \date Mar 7, 2009
/// \author Mark Garlanger
///

#ifndef MEMORY_H_
#define MEMORY_H_

#include "config.h"
#include "h89Types.h"

///
/// \brief Abstract physical memory base class
///
/// This is the abstract parent class to handle both RAM and ROM.
///
class Memory
{
  protected:
    WORD baseAddress_m;
    int  size_m;

  public:

    ///
    ///   \param size Size of the memory.
    ///
    Memory(int size);
    virtual ~Memory();

    ///
    /// \brief Write a byte to memory.
    ///
    /// \param addr address to write to.
    /// \param val value to write.
    ///
    virtual void writeByte(WORD addr, BYTE val) = 0;


    ///
    /// \brief Read byte from memory.
    ///
    /// \param addr address to read from.
    ///
    /// \retval Value read from the memory.
    ///
    virtual BYTE readByte(WORD addr) = 0;

    ///
    /// \brief Get Base Address of the memory device.
    ///
    virtual WORD getBaseAddress(void);

    ///
    /// \brief Set the base address of the memory device
    ///
    /// \param addr Base address for the memory.
    virtual void setBaseAddress(WORD addr);

    ///
    /// \brief Returns the size of the memory.
    ///
    /// \retval The size of the memory.
    ///
    virtual int  getSize(void);
};

#endif // MEMORY_H_
