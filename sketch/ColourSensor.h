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
  Colour m_targetCol;
  
  // TODO: add data needed for colour sensor
};

#endif