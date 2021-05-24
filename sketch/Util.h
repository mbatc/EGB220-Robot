#ifndef Utility_h__
#define Utility_h__

#include "Arduino.h"
#include "math.h"
#include "stdio.h"
#include "stdint.h"

// placement new
void * operator new (size_t size, void * ptr);

#define ArraySize(arr) (sizeof(arr) / sizeof(*arr))

#define PRINT_DEBUG_INFO // Comment this line to disable debug Serial prints

#ifdef PRINT_DEBUG_INFO
  #define DEBUG_PRINT(val) Serial.print(val)
  #define DEBUG_PRINTLN(val) Serial.println(val)
#else
  #define DEBUG_PRINT(val)
  #define DEBUG_PRINTLN(val)
#endif

extern char const * whitespace;

// A floating pointer version of arduinos map() function
double mapf(double x, double inMin, double inMax, double outMin, double outMax);

#define SIGN(val) (val < 0 ? -1 : 1)
#define FLT_MAX 3.402823466E+38

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

// Simple way to get a unique integer for each type
int __NextTypeID();

template<typename T> int TypeID() { static int id = __NextTypeID(); return id; }

char const * TypeName(...);
char const * TypeName(float*);
char const * TypeName(double*);
char const * TypeName(int*);
char const * TypeName(bool*);

template<typename T> char const * TypeName() { return TypeName((T*)0); }

void rollingAverage(double *pAverage, double newSample, int nSamples);
void expMovingAverage(double *pAverage, double newSample, int nSamples, double smoothing);

template<typename T>
void __printArgs(T&& value)
{
  DEBUG_PRINT(" ");
  DEBUG_PRINT(value);
}

template<typename T, typename... Args>
void __printArgs(T&& first, Args&&... args)
{
  __printArgs(util::forward<T>(first));
  __printArgs(util::forward<Args>(args)...);
}

template<typename... Args>
void debugPrint(char const *name, Args&&... args)
{
  DEBUG_PRINT(name);
  DEBUG_PRINT(": [");
  __printArgs(util::forward<Args>(args)...);
  DEBUG_PRINT(" ] ");
}

inline void debugPrint(char const *name)
{
  DEBUG_PRINT(name);  
  DEBUG_PRINT(" ");
}

#endif
