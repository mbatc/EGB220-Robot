#ifndef Utility_h__
#define Utility_h__

#include "math.h"
#include "Arduino.h"

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

// Simple way to get a unique integer for each type
int __NextTypeID();

template<typename T> int TypeID() { static int id = __NextTypeID(); return id; }

char const * TypeName(...);
char const * TypeName(float*);
char const * TypeName(double*);
char const * TypeName(int*);
char const * TypeName(bool*);

template<typename T> char const * TypeName() { return TypeName((T*)0); }

#endif
