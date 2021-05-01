#include "stdio.h"
#include "stdint.h"
#include <new.h>

#define INVALID_INDEX UINT32_MAX

// This namespace contains some utility functions for initializing, copying
// and moving objects, and arrays of objects.
namespace util
{
  template<typename T> struct identity { typedef T type; };

  template<typename T> T &&forward(typename identity<T>::type &o) { return static_cast<typename identity<T>::type &&>(o); }
  template<typename T> T &&move(T &o)                             { return static_cast<T&&>(o); }
  template<typename T> T   copy(T const &o)                       { return T(o); }

  template<typename T, typename... Args>
  void init(T *pMem, Args&&... args) {
    new (pMem) T(forward<Args>(args)...);
  }

  template<typename T> void destruct(T *pValue) { pValue->~T(); }

  template<typename T, typename... Args>
  void initArray(T *pData, size_t count, Args&&... args) {
    while (count-- != 0)
      init(pData++, forward<Args>(args)...);
  }

  template<typename T>
  void destructArray(T *pData, size_t count) {
    while (count-- != 0)
      destruct(pData++);
  }

  template<typename T>
  void initArrayCopy(T *pDest, T const *pSrc, size_t count) {
    while (count-- != 0)
      init(pDest++, *(pSrc++));
  }

  // Init an array iterating forward
  template<typename T>
  void initArrayMove(T *pDest, T *pSrc, size_t count) {
    while (count-- != 0)
      init(pDest++, move(*(pSrc++)));
  }

  // Init an array iterating backward
  template<typename T>
  void initArrayMoveReversed(T *pDest, T *pSrc, size_t count) {
    pDest = pDest + count;
    pSrc = pSrc + count;
    while (count-- != 0)
      init(--pDest, move(*(--pSrc)));
  }
}

template<typename T>
class List
{
public:
  ~List()
  {
    clear();
  }

  size_t size()     const { return m_size; }
  size_t capacity() const { return m_capacity; }

    // Append a value to the back of the list
  bool append(T value) {
    return insert(value, size());
  }

  // Insert at index
  bool insert(T value, size_t index) {
    if (index < 0 || index > size())
      return false;
    if (!tryGrow(size() + 1))
      return false;

    // A pointer to the existing element
    T *pNewValue = &get(index);

    // Move elements to make space for the new item
    util::initArrayMoveReversed(pNewValue + 1, pNewValue, size() - index);

    // Move the new element into the array
    *pNewValue = util::move(value);
    ++m_size;
    return true;
  }

  // Erase at index.
  // Returns false if the index is invalid.
  // Returns true if the item was successfully removed.
  bool erase(size_t index, size_t count = 1) {
    if (!valid(index))
      return false;

    // Destroy the item in the list
    T *pItem = &get(index);
    util::destruct(pItem);

    // Shift remaining elements to the left
    util::initArrayMove(pItem, pItem + 1, size() - index - 1);
    --m_size;
    return true;
  }

  // Resize the list.
  // Returns false if memory allocation fails
  // Returns true on success.
  bool resize(size_t newSize) { return resize(size, T()); }

  // Resize the list.
  // Returns false if memory allocation fails
  // Returns true on success.
  bool resize(size_t newSize, T const &initialValue) {
    // Make sure we have the required capacity
    if (!tryGrow(newSize))
      return false;
    util::initArray(end(), newSize - size, initialValue);
    return true;
  }

  // Returns true the index is valid
  bool valid(size_t index) { return index >= 0 && index < size(); }

  // Erase everything from the array
  void clear() {
    while (size() > 0)
      erase(size() - 1);

    free(m_pData);
    m_capacity = 0;
  }

  // Remove the item at the back of the list
  // The list must contain at least 1 item
  void popBack() { erase(size() - 1); }

  // Remove the item at the front of the list
  // The list must contain at least 1 item
  void popFront() { erase(0); }

  // Find the index of an item in the list
  // Returns INVALID_INDEX if the item cannot be found
  size_t find(T const &value) {
    for (uint32_t i = 0; i < size(); ++i)
      if (get(i) == value)
        return i;
    return INVALID_INDEX;
  }

  // Get a value in the list
  T const &get(size_t index) const { return data()[index]; }
  T &get(size_t index) { return data()[index]; }

  // Array access operators
  T const &operator[](size_t index) const { return get(index); }
  T &operator[](size_t index) { return get(index); }

  T const *data() const { return m_pData; }
  T *data() { return m_pData; }

  T *begin() { return data(); }
  T *end() { return data() + size(); }
  T const *begin() const { return data(); }
  T const *end()   const { return data() + size(); }

private:
  // Reallocate the internal array to so that there is space for
  // 'newCapacity' items
  bool tryGrow(size_t newCapacity) {
    if (newCapacity <= m_capacity)
      return true;

    size_t sizeInBytes = newCapacity * sizeof(T);
    T *pNewBlock = (T *)realloc(m_pData, sizeInBytes);
    if (pNewBlock == nullptr)
    { // Re-alloc failed, allocate a new block of memory
      pNewBlock = (T *)malloc(sizeInBytes);
      if (pNewBlock == nullptr)
        return false; // Out of memory
      util::initArrayMove(pNewBlock, m_pData, size());
      free(m_pData);
    }

    m_pData = pNewBlock;
    m_capacity = newCapacity;
    return true;
  }

  T *m_pData = nullptr;  // The allocated array
  size_t m_size = 0;     // The number of items in the list
  size_t m_capacity = 0; // The number of items that can be added until the array needs to be reallocated
};
