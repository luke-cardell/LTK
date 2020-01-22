/*! \file       LTK_ScratchPad.h
**  \author     Lux Cardell
**  \date       10/14/2018
**  \brief
**    Short-term memory allocator tool.
**
**  \copyright  GNU Public License.
*/
#ifndef LTK_SCRATCHPAD_H
#define LTK_SCRATCHPAD_H

#include <utility>

using size_t = std::size_t;

//! Lux's Tool Kit
namespace LTK {
  /*! ScratchPad
  **  \brief
  **    ScratchPad for short-term allocations.
  **  \tparam PadSize
  **    The size of the ScratchPad's memory space.
  **  \par About:
  **    This ScratchPad is designed with the
  **    following requirements:
  **    + No memory allocations are made by the
  **      allocator any point. ("allocation", as
  **      used by the rest of this class's
  **      documentation refers to reallocation of
  **      the existing memory in the ScratchPad)
  **    + Memory allocated from the ScratchPad is
  **      meant to be temporary and short term.
  **      That means that memory allocated should
  **      never be deleted, as it is never
  **      actually returned to the OS for reuse.
  **      Instead, it is reused by the ScratchPad.
  **    + The allocator does not throw exceptions
  **      at any point. If the allocator cannot
  **      fit a requested allocation size, it
  **      returns null.
  **    + Given the nature of the allocations and
  **      the memory itself, the ScratchPad should
  **      not be copy constructable, copy
  **      assignable, move constructable, or move
  **      assignable.
  **    .
  */
  template<size_t PadSize>
  class ScratchPad {
  public:
    ~ScratchPad() = default;
    ScratchPad(ScratchPad const&) = delete;
    ScratchPad(ScratchPad &&) = delete;
    ScratchPad & operator=(ScratchPad const&) = delete;
    ScratchPad & operator=(ScratchPad &&) = delete;
    
    /*! Constructor
    **  \brief
    **    Default constructor
    */
    ScratchPad()
      : m_empty(PadSize),
        m_write(&m_pad[0]),
        m_pad{0}
#ifdef LTK_DEBUG_SCRATCHPAD
        , d_allocs(0)
#endif // LTK_DEBUG_SCRATCHPAD
    {}
    
    /*! Clear
    **  \brief
    **    Clears the ScratchPad's allocated memory
    **    for rewriting.
    */
    void Clear() {
      // reset the write pointer
      m_write = &m_pad[0];
      // reset the empty memory
      m_empty = PadSize;
#ifdef LTK_DEBUG_SCRATCHPAD
      // reset the allocations
      d_allocs = 0;
#endif // LTK_DEBUG_SCRATCHPAD
    }
    
    /*! Alloc
    **  \brief
    **    Allocates a block of memory from the
    **    pad.
    **  \param block
    **    The size of the block to allocate.
    **  \param count
    **    The number of blocks to allocate.
    **    Defaults to 1.
    **  \return
    **    Returns the address of the allocation.
    **    Returns null if the allocation fails or
    **    if the block count is 0.
    */
    void * Alloc(size_t block, size_t count = 1) {
      if (count == 0) return nullptr;
      // the pointer to return
      void * result = nullptr;
      // check if the type can fit
      if (block * count <= m_empty)
      {
#ifdef LTK_DEBUG_SCRATCHPAD
        // increment the allocations
        ++d_allocs;
#endif // LTK_DEBUG_SCRATCHPAD
        // allocate from the pad
        m_empty -= block * count;
        // set the result pointer
        result = m_write;
        // update the write pointer
        m_write += block * count;
      }
      // return the result
      return result;
    }
    
    /*! Space
    **  \brief
    **    Getter function for the empty space left
    **    in the pad.
    **  \return
    **    Returns the number in bytes.
    */
    size_t Space() const {
      return m_empty;
    }
    
    /*! Data
    **  \brief
    **    Accessor for the internal data pad.
    **  \return
    **    Returns a pointer to the internal data.
    */
    char * Data() {
      return m_pad;
    }
    
#ifdef LTK_DEBUG_SCRATCHPAD
    /*! Allocations
    **  \brief
    **    Function to check the number of entries
    **    made since the last call to clear.
    **  \return
    **    Returns the number of allocations.
    */
    size_t Allocations() const {
      return d_allocs;
    }
#endif // LTK_DEBUG_SCRATCHPAD
    
  private:
    //! size of the empty memory
    size_t m_empty;
    //! beginning of empty memory
    char * m_write;
    //! wite pad
    char m_pad[PadSize];
#ifdef LTK_DEBUG_SCRATCHPAD
    //! number of allocations
    size_t d_allocs;
#endif // LTK_DEBUG_SCRATCHPAD
  };

} // Lux's Tool Kit

#endif // LTK_SCRATCHPAD_H

/* Authored by Lux Cardell
** Open source under GNU Public License
*/
