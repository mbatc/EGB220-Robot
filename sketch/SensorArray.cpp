#include "SensorArray.h"

void SensorArray::Sensor::setPin(int pin) {
  m_pin = pin;
  pinMode(m_pin, INPUT);
}  

int SensorArray::Sensor::read() {
  m_rawValue = analogRead(m_pin);

  if (m_calibrating)
  { // Update the range of sensor values
    m_min = min(m_min, m_rawValue);
    m_max = max(m_max, m_rawValue);
  }

  // Remap the sensor value
  m_value = map(m_rawValue, m_min, m_max, SENSOR_MIN, SENSOR_MAX);

  // clamp the sensor value
  m_value = min(max(m_value, SENSOR_MIN), SENSOR_MAX);
}

void SensorArray::Sensor::resetCalibration() {
  m_min = 1024;
  m_max = 0;
}

void SensorArray::Sensor::setCalibrating(bool calibrating) {
  if (!m_calibrating && calibrating)
    resetCalibration();
  m_calibrating = calibrating;
}

int SensorArray::Sensor::getMin()      const { return m_min; }
int SensorArray::Sensor::getMax()      const { return m_max; }
int SensorArray::Sensor::getValue()    const { return m_value; }
int SensorArray::Sensor::getValueRaw() const { return m_rawValue; }

bool SensorArray::setup(SensorConfig conf) {
  // Store the config settings
  m_config = conf;
  
  // Setting pins as inputs or outputs depending on what they are
  pinMode(m_config.emitPin, OUTPUT);
  digitalWrite(m_config.emitPin, HIGH);
  
  // Setting IR receivers as inputs
  for (int i = 0; i < 8; i++)
    m_sensors[i].setPin(m_config.recvPins[i]);
}

void SensorArray::update() {
  updateSensorValues();
  updateLinePosition();
}

void SensorArray::updateLinePosition() {
  // more debugging serial prints
  DEBUG_PRINT("     ir receiver values:");
  for (Sensor &sensor : m_sensors) {
    DEBUG_PRINT("  ");
    DEBUG_PRINT(sensor.getValue());
  }
  
  DEBUG_PRINT("    Sensor Avg: ");
  DEBUG_PRINT(m_irAvg);
  DEBUG_PRINT("    Sensor Std-Dev: ");
  DEBUG_PRINT(m_irStdDev);

  // If a horizontal line is detected, keep doing what ya doing
  if (horizontalLineDetected()) {
    DEBUG_PRINT("    Horizontal Detected");
    return;
  }
  
  // If no line is detected, keep doing what ya doing
  if (!lineDetected()) {
    DEBUG_PRINT("    No Line Detected");
    return;
  }

  // Take a weighted average of the IR sensor indices.
  m_linePosition = 0;
  double totalWeight = 0; // Store total weighting applied for all variables
  for (int i = 0; i < IR_SENSOR_COUNT; ++i) {
    Sensor &sensor = m_sensors[i];
    // Calculate the weight for the sensor reading at 'i'
    // The weight is based on how different the sensor value is from the minimum value recorded
    double dist = abs(mapf(sensor.getValue(), SENSOR_MIN, SENSOR_MAX, 0, 1));
    // Adjust the weight using a power function.
    // This will more heavily weight values closer to m_irMin.
    double weight = (1.0 - pow(dist, m_config.sensorFalloff));
    m_linePosition += i * weight; // Sum the weighted indices
    totalWeight += weight;        // Sum the weights so we can divide by the total later
  }
  m_linePosition /= totalWeight; // Calculate the weighted average.

  // de-bugging serial print, can be comented out
  DEBUG_PRINT("     Line Pos: ");
  DEBUG_PRINT(m_linePosition);

  rollingAverage(&m_averageLinePosition, m_linePosition, m_averageSampleCount);
}

void SensorArray::updateSensorValues() {
  // Read in all the sensor values
  for (Sensor &sensor : m_sensors)
    sensor.read();

  // Calculate the average value for all sensors.
  // Used to differentiate between no-line detected and a horizontal line detected.
  m_irAvg = 0;
  for (Sensor &sensor : m_sensors)
    m_irAvg += sensor.getValue();
  m_irAvg /= IR_SENSOR_COUNT;

  // Calculate the standard deviation in sensor values.
  // Used to determine if the sensors are detecting a line or not.
  double sum = 0;
  for (Sensor &sensor : m_sensors) {
    sum += sq(double(sensor.getValue() - m_irAvg));
  }
  m_irStdDev = sqrt(sum / IR_SENSOR_COUNT);

  // Record how long the line has been detected or missing for.
  int currentTime = millis();
  int dt = currentTime - m_lastUpdateMilli; // Get the number of milliseconds since the sensors were last updated.
  if (lineDetected()) {
    m_lineMissingMilli   = 0; // Clear missing time and increment detected time.
    m_lineDetectedMilli += dt;
  }
  else {
    m_lineMissingMilli += dt; // Increment missing time and clear detected time.
    m_lineDetectedMilli = 0;
  }
  m_lastUpdateMilli = currentTime; // Store the current time (will be used in the call to this function).
  
  DEBUG_PRINT("    Detected Time: ");
  DEBUG_PRINT(m_lineDetectedMilli);
  DEBUG_PRINT("    Missing Time: ");
  DEBUG_PRINT(m_lineMissingMilli);
}

double SensorArray::getLinePos()    { return m_linePosition; }
double SensorArray::getLinePosRaw() { return m_averageLinePosition; }

void SensorArray::setAverageSampleCount(int sampleCount) { m_averageSampleCount = sampleCount; }
bool SensorArray::lineDetected() { return m_irStdDev > m_config.detectThreshold; }
bool SensorArray::horizontalLineDetected() { return !lineDetected() && m_irAvg < 100; }

// Get the number of milliseconds the line has not been detected continuosly.
int SensorArray::lineMissingTime() { return m_lineMissingMilli; }

// Get the number of milliseconds the line has been detected continuosly.
int SensorArray::lineDetectedTime() { return m_lineDetectedMilli; }
