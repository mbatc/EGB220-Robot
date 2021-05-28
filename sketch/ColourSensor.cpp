#include "ColourSensor.h"
#include "arduino.h"

#define COLOURS2 A4
#define COLOURS3 A3

ColourSensor::ColourSensor()
{
  m_outPin = 15;
  m_signal2 = 16;
  m_signal3 = 14;

  pinMode(m_signal2, OUTPUT);
  pinMode(m_signal3, OUTPUT);

  // Detect RED
  digitalWrite(m_signal2,LOW);
  digitalWrite(m_signal3,LOW);
}

int ColourSensor::getIntensity() {
  //measure intensity with oversampling
  int a = 0;
  int b = 255;
  
  for(int i=0;i<10;i++)
    a=a+pulseIn(m_outPin, LOW);
    
  if(a>9)
    b=2550/a;
    
  return b;
}

Colour ColourSensor::getColour() const
{
  if (isRed())        return Col_Red;
  else if (isGreen()) return Col_Green;
  else if (isBlack()) return Col_Black;
  else if (isWhite()) return Col_White;
  else return Col_None;
}

bool ColourSensor::isDetected(Colour col) const
{
  switch (col)
  {
    case Col_Red:   return isRed();
    case Col_Green: return isGreen();
  }
  
  return false;
}

void ColourSensor::update()
{
  m_intensity = getIntensity();
}

bool ColourSensor::isGreen() const
{
  return m_intensity > 8 && m_intensity < 16;
}

bool ColourSensor::isRed() const
{
  return m_intensity > 20 && m_intensity < 35;
}

bool ColourSensor::isWhite() const
{
  return m_intensity > 45;
}

bool ColourSensor::isBlack() const
{
  return m_intensity < 6;
}
