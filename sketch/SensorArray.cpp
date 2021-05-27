#include "SensorArray.h"

bool g_calibrateSensors = false;

void SensorArray::Sensor::setPin(int pin) {
  m_pin = pin;
  pinMode(m_pin, INPUT);
}  

int SensorArray::Sensor::read() {
  m_rawValue = analogRead(m_pin);

  if (g_calibrateSensors)
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

int SensorArray::Sensor::getMin()      const { return m_min; }
int SensorArray::Sensor::getMax()      const { return m_max; }
int SensorArray::Sensor::getValue()    const { return m_value; }
int SensorArray::Sensor::getValueRaw() const { return m_rawValue; }

bool SensorArray::setup(SensorConfig conf) {
  // Store the config settings
  m_config = conf;
    
  // Setting IR receivers as inputs
  for (int i = 0; i < 8; i++)
    m_sensors[i].setPin(m_config.recvPins[i]);

  m_markers[MS_Left].setPin(m_config.leftMarkerPin);
  m_markers[MS_Right].setPin(m_config.rightMarkerPin);

  m_averageSampleCount = 1;
}

void SensorArray::update() {
  updateSensorValues();
  updateLinePosition();
}

void SensorArray::updateLinePosition() {
  // more debugging serial prints
  // DEBUG_PRINT("ir array: [ ");
  // for (Sensor &sensor : m_sensors) {
  //   DEBUG_PRINT(sensor.getValue());
  //   DEBUG_PRINT(" ");
  // }
  // DEBUG_PRINT("] ");
  
  // debugPrint("markers", m_markers[0].getValue(), m_markers[1].getValue());
  
  // If a horizontal line is detected, keep doing what ya doing
  if (horizontalLineDetected()) {
    // debugPrint("Horizontal Detected");
    return;
  }
  
  // If no line is detected, keep doing what ya doing
  if (!lineDetected()) {
    // debugPrint("No Line Detected");
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
  rollingAverage(&m_averageLinePosition, m_linePosition, m_averageSampleCount);
}

void SensorArray::updateSensorValues() {
  // Read in all the sensor values
  for (Sensor &sensor : m_sensors)
    sensor.read();

  for (Sensor &marker : m_markers)
    marker.read();

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
  
  // debugPrint("Detected Time", m_lineDetectedMilli);
  // debugPrint("Missing Time", m_lineMissingMilli);
}

bool SensorArray::isMarkerDetected(MarkerSensor sensorID)
{
  return m_markers[sensorID].getValue() < 512;
}

void SensorArray::resetCalibration()
{
  for (Sensor &sensor : m_sensors)
    sensor.resetCalibration();
  for (Sensor &sensor : m_markers)
    sensor.resetCalibration();
}

double SensorArray::getLinePos()    { return m_linePosition / (IR_SENSOR_COUNT - 1); }

bool SensorArray::lineDetected() { return m_irStdDev > m_config.detectThreshold; }
bool SensorArray::horizontalLineDetected() { return !lineDetected() && m_irAvg < 100; }

// Get the number of milliseconds the line has not been detected continuosly.
int SensorArray::lineMissingTime() { return m_lineMissingMilli; }

// Get the number of milliseconds the line has been detected continuosly.
int SensorArray::lineDetectedTime() { return m_lineDetectedMilli; }
