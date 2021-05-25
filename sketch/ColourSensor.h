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

  void SetTarget(Colour col);

  bool IsDetected() const;

  Colour GetTarget() const;

private:
  int getIntensity();

  Colour m_targetCol;
  int m_outPin = -1;
  int m_signal2 = -1;
  int m_signal3 = -1;
};

#endif
