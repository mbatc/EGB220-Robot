#include "SensorArray.h"

bool SensorArray::setup(SensorConfig conf)
{
  // Store the config settings
  m_config = conf;
  
  // Setting pins as inputs or outputs depending on what they are
  pinMode(m_config.emitPin, OUTPUT);
  digitalWrite(m_config.emitPin, HIGH);
  
  // Setting IR receivers as inputs
  for (int i = 0; i < 8; i++) {
    pinMode(m_config.recvPins[i], INPUT);
  }
}

double SensorArray::getLinePos() {  
  return m_linePosition; // Line position calculated in updateLinePosition()
}

bool SensorArray::lineDetected() {
  return m_irStdDev > m_config.detectThreshold; // High standard deviation means line is detected.
}

bool SensorArray::horizontalLineDetected() {
  return !lineDetected() && m_irAvg < 100; // No line, but low average sensor reading (threshold could possibly be added to the config struct)
}

void SensorArray::update() {
  updateSensorValues();
  updateLinePosition();
}

void SensorArray::updateLinePosition() {
  // more debugging serial prints
  DEBUG_PRINT("     ir receiver values:");
  for (int j = 0; j < IR_SENSOR_COUNT; j++) {
    DEBUG_PRINT("  ");
    DEBUG_PRINT(m_irValue[j]);
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
    
  // double num = 0;
  // double den = 0;
  // for (int i = 0; i < IR_SENSOR_COUNT; ++i) {
  //  numerator   += (i + 1) * irValue[i];
  //  denomenator += irValue[i];
  // }

  // Take a weighted average of the IR sensor indices.
  m_linePosition = 0;
  double totalWeight = 0; // Store total weighting applied for all variables
  for (int i = 0; i < IR_SENSOR_COUNT; ++i) {
    // Calculate the weight for the sensor reading at 'i'
    // The weight is based on how different the sensor value is from the minimum value recorded
    double dist = abs(mapf(m_irValue[i], m_irMin, m_irMax, 0, 1));
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
}

void SensorArray::updateSensorValues() {
  // Clear min/max values
  m_irMin = 1024;
  m_irMax = 0;

  // Read in all the sensor values
  for (int j = 0; j < IR_SENSOR_COUNT; j++) {
    m_irValue[j] = analogRead(m_config.recvPins[j]);
    m_irMin = min(m_irMin, m_irValue[j]); // Get the min/max while we are at it
    m_irMax = max(m_irMax, m_irValue[j]);
  }

  // Calculate the average value for all sensors.
  // Used to differentiate between no-line detected and a horizontal line detected.
  m_irAvg = 0;
  for (int val : m_irValue) {
    m_irAvg += val;
  }
  m_irAvg /= IR_SENSOR_COUNT;

  // Calculate the standard deviation in sensor values.
  // Used to determine if the sensors are detecting a line or not.
  double sum = 0;
  for (int val : m_irValue) {
    sum += sq(double(val - m_irAvg));
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

// Get the number of milliseconds the line has not been detected continuosly.
int SensorArray::lineMissingTime() {
  return m_lineMissingMilli;
}

// Get the number of milliseconds the line has been detected continuosly.
int SensorArray::lineDetectedTime() {
  return m_lineDetectedMilli;
}
