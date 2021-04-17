#ifndef SensorArray_h__
#define SensorArray_h__

#include "Util.h"

#define IR_SENSOR_COUNT  6

// Struct that contains configuration options for the sensor array
struct SensorConfig
{
  int emitPin;
  int recvPins[IR_SENSOR_COUNT];
  int detectThreshold;
  double sensorFalloff;
};

class SensorArray
{
public:
  // Setup the sensor array using the given configuration
  bool setup(SensorConfig conf);

  // function to determine where the line is
  double getLinePos();

  // Returns true if a line is being detected.
  // Returns false if the all sensor readings are similar.
  //   e.g. If they are all over a black surface, or all over a white surface.
  //
  // How sensitive the line detection is can be configured by
  // changing 'detectThreshold' in the SensorConfig struct.
  //
  // Increasing detectThreshold will make it less sensitive.
  // Decreasing detectThreshold will make it more sensitive.
  //
  // If a horizontal line is detected, this will also return false.
  bool lineDetected();

  // Check if the sensor array has detected a horizontal line.
  bool horizontalLineDetected();

  // Get the number of milliseconds the line has not been detected continuosly.
  int lineMissingTime();

  // Get the number of milliseconds the line has been detected continuosly.
  int lineDetectedTime();

  // This function reads the sensor and calculates the Derived IR sensor values below.
  // Should be called once per loop().
  void update();

  // Set the calibration mode of the sensor array.
  void setCalibrating(bool mode);

  // Returns true if the sensor array is being calibrated.
  bool isCalibrated();

  // Reset the calibration of the sensor array
  void resetCalibration();
  
protected:
  void updateSensorValues();
  void updateLinePosition();

  // Raw IR sensor values
  int m_irValue[IR_SENSOR_COUNT];
  int m_irCalibMin[IR_SENSOR_COUNT];
  int m_irCalibMax[IR_SENSOR_COUNT];
  
  // Derived IR sensor values
  int m_irAvg    = 0; // Average sensor value
  int m_irStdDev = 0; // Standard Deviation in sensor values
  int m_irMin    = 0; // The minimum sensor value
  int m_irMax    = 0; // The maximum sensor value

  // Line detection state
  double m_linePosition   = 0;
  int m_lineDetectedMilli = 0;
  int m_lineMissingMilli  = 0;
  int m_lastUpdateMilli   = 0;

  // Sensor array configuration
  SensorConfig m_config;

  bool m_calibrating = false;
  bool m_calibrated = true;
};

#endif
