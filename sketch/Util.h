#ifndef Utility_h__
#define Utility_h__

#include "math.h"
#include "Arduino.h"

#define PRINT_DEBUG_INFO // Comment this line to disable debug Serial prints

#ifdef PRINT_DEBUG_INFO
  #define DEBUG_PRINT(val) Serial.print(val)
  #define DEBUG_PRINTLN(val) Serial.println(val)
#else
  #define DEBUG_PRINT(val)
  #define DEBUG_PRINTLN(val)
#endif

// A floating pointer version of arduinos map() function
double mapf(double x, double inMin, double inMax, double outMin, double outMax);

#define SIGN(val) (val < 0 ? -1 : 1)

#endif
