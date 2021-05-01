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
  *pAverage = ((*pAverage * (nSamples - 1)) + newSample) / nSamples;
}

void expMovingAverage(double *pAverage, double newSample, int nSamples, double smoothing)
{
  return newSample * (smoothing / (1 + nSamples)) + (*pAverage) * (1 - (smoothing / (1 + nSamples)));
}
