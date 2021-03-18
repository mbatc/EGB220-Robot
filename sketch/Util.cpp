#include "Util.h"

double mapf(double x, double inMin, double inMax, double outMin, double outMax)
{
  return ((x - inMin) / (inMax - inMin)) * (outMax - outMin) + outMin;
}
