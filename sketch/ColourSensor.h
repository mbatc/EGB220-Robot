#ifndef ColourSensor_h__
#define ColourSensor_h__

enum Colour
{
  Col_Red,
  Col_Green,
  Col_Count
};

class ColourSensor
{
public:
  ColourSensor();

  void update();

  bool isDetected(Colour col) const;
  bool isGreen() const;
  bool isRed() const;
  bool isWhite() const;
  bool isBlack() const;

  int getIntensity();
  
private:
  int m_intensity;

  int m_outPin = -1;
  int m_signal2 = -1;
  int m_signal3 = -1;
};

#endif
