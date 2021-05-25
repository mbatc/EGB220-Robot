#include "ColourSensor.h"

ColourSensor::ColourSensor()
{

}

void ColourSensor::SetTarget(Colour colour)
{
  // Todo: configure colour sensor to detect selected colour
}

bool ColourSensor::IsDetected() const
{
  // ToDo: implement colour detection
  return false; 
}

Colour ColourSensor::GetTarget() const { return m_targetCol; }
