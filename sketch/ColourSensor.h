#ifndef ColourSensor_h__
#define ColourSensor_h__

enum Colour
{
  Col_None,
  Col_Colour,
  Col_White,
  Col_Black,
  Col_Count
};

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
