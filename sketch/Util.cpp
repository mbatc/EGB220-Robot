#include "Util.h"

double mapf(double x, double inMin, double inMax, double outMin, double outMax)
{
  return ((x - inMin) / (inMax - inMin)) * (outMax - outMin) + outMin;
}

int __NextTypeID() { static int nextID; return nextID++; }

char const * TypeName(float*)   { return "f32";  }
char const * TypeName(double*)  { return "f64";  }
char const * TypeName(int*)     { return "i32";  }
char const * TypeName(bool*)    { return "b";    }
char const * TypeName(...)      { return "none"; }

char const * whitespace = " \n\r\t";

void rollingAverage(double *pAverage, double newSample, int nSamples)
{
  *pAverage = (((nSamples - 1) * (*pAverage)) + newSample) / nSamples;
}