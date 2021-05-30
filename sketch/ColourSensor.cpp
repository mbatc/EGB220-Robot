#include "ColourSensor.h"
#include "arduino.h"

#define COLOURS2 A4
#define COLOURS3 A3

Colour getColour(int intensity)
{
  if (intensity < 12) {
    return Col_Black;
  }
  else if (intensity >= 12 && intensity < 25) {
    return Col_Green;
  }
  else if (intensity >= 25 && intensity < 35) {
    return Col_Red;
  }
  else if (intensity >= 35) {
    return Col_White;
  }
  
  return Col_None;
}

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

void ColourSensor::update()
{
  m_intensity = getIntensity();
}
