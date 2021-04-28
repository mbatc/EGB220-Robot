#ifndef SensorArray_h__
#define SensorArray_h__

#include "Util.h"

#define IR_SENSOR_COUNT 8

#define SENSOR_MIN 0
#define SENSOR_MAX 1024

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
  class Sensor
  {
  public:
    void setPin(int pin);
    
    int getMin() const;
    int getMax() const;
    int getValue() const;
    int getValueRaw() const;
    int read();

    void resetCalibration();
    void setCalibrating(bool calibrating);

  protected:
    int m_pin = 0;
    int m_min = 0;    // The minimum sensor value
    int m_max = 1024; // The maximum sensor value
    int m_value = 0;
    int m_rawValue = 0;
    bool m_calibrating = false;
  };

public:
  // Setup the sensor array using the given configuration
  bool setup(SensorConfig conf);

  // function to determine where the line is
  double getLinePos();

  // Calculate where the line is
  double getLinePosRaw();

  // Set the number of samples used to calculate the average line position
  void setAverageSampleCount(int sampleCount);

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
  
protected:
  void updateSensorValues();
  void updateLinePosition();

  // Raw IR sensor values
  Sensor m_sensors[IR_SENSOR_COUNT];

  // Derived IR sensor values
  int m_irAvg    = 0; // Average sensor value
  int m_irStdDev = 0; // Standard Deviation in sensor values

  // Line detection state
  double m_linePosition        = 0;
  double m_averageLinePosition = 0;
  int m_averageSampleCount     = 10;
  int m_lineDetectedMilli = 0;
  int m_lineMissingMilli  = 0;
  int m_lastUpdateMilli   = 0;

  // Sensor array configuration
  SensorConfig m_config;
};

#endif
