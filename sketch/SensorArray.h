#ifndef SensorArray_h__
#define SensorArray_h__

#define IR_SENSOR_COUNT 8

struct SensorConfig
{
  int emitPin;
  int recvPins[IR_SENSOR_COUNT];
  int detectThreshold;
};

class SensorArray
{
public:
  // Setup the sensor array using the given configuration
  bool setup(SensorConfig conf);

  // function to determine where the line is
  double getLinePos();

  // Returns true if the line is being detected.
  // TODO: Document reasons this might return false.
  bool lineDetected();

  // Returns true if all the sensors detect the line
  bool horizontalLineDetected();

protected:
  void updateSensorValues();

  // IR sensor values
  int m_irValue[IR_SENSOR_COUNT];
  int m_irAvg    = 0;
  int m_irStdDev = 0;
  int m_irMin    = 0;
  int m_irMax    = 0;

  // Line detection state
  double m_linePosition = 0;
  
  // Sensor array config
  SensorConfig m_config;
};

#endif
