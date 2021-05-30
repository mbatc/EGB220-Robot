#ifndef ColourSensor_h__
#define ColourSensor_h__

enum Colour
{
  Col_None,
  Col_Red,
  Col_Green,
  Col_White,
  Col_Black,
  Col_Count
};

Colour getColour(int intensity);

class ColourSensor
{
public:
  ColourSensor();
  void update();
  int getIntensity();
  
private:
  volatile int m_intensity;

  volatile int m_outPin = -1;
  volatile int m_signal2 = -1;
  volatile int m_signal3 = -1;
};

#endif
