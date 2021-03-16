#include "SensorArray.h"
#include "math.h"
#include "Arduino.h"
#define PRINT_DEBUG_INFO
// the max and min sensorArray values determine how aggresive the rampdown speed of the motors are when it sees a corner. the closer these are to the upper and lower bound, the 'faster' the robot will try to corner.
double maxSensorArray = 65000;
double minSensorArray = 25000;

bool SensorArray::setup(SensorConfig conf)
{
  m_config = conf;
  
  //setting pins as inputs or outputs depending on what they are
  pinMode(m_config.emitPin, OUTPUT);
  digitalWrite(m_config.emitPin, HIGH);
  
  //ir receivers are inputs
  for (int i = 0; i < 8; i++) {
    pinMode(m_config.recvPins[i], INPUT);
  }
}

double SensorArray::getLinePos() {
  updateSensorValues();
  
  // more debugging serial prints
  Serial.print("     ir receiver values:");
  for (int j = 0; j < IR_SENSOR_COUNT; j++) {
    Serial.print("  ");
    Serial.print(m_irValue[j]);
  }
  
  Serial.print("    Sensor Avg: ");
  Serial.print(m_irAvg);
  Serial.print("    Sensor Std-Dev: ");
  Serial.print(m_irStdDev);
    
  if (horizontalLineDetected()) {
    Serial.print("    Horizontal Detected");
    return m_linePosition;
  }
  
  if (!lineDetected()) {
    Serial.print("    No Line Detected");
    return m_linePosition;
  }
    
  // double num = 0;
  // double den = 0;
  // for (int i = 0; i < IR_SENSOR_COUNT; ++i) {
  //  numerator   += (i + 1) * irValue[i];
  //  denomenator += irValue[i];
  // }
  
  m_linePosition = 0;
  double totalWeight = 0;
  for (int i = 0; i < IR_SENSOR_COUNT; ++i) {
    double dist = double(abs(m_irValue[i] - m_irMin)) / (m_irMax - m_irMin);
    double weight = (1.0 - pow(dist, 0.25));
    m_linePosition += i * weight;
    totalWeight += weight;
  }
  m_linePosition /= totalWeight;

  #ifdef PRINT_DEBUG_INFO
  // de-bugging serial print, can be comented out
  Serial.print("     Line Pos: ");
  Serial.print(m_linePosition);
  #endif
  
  return m_linePosition;
}

bool SensorArray::lineDetected() {
  return m_irStdDev > m_config.detectThreshold;
}

bool SensorArray::horizontalLineDetected() {
  return !lineDetected() && m_irAvg < 100;
}

void SensorArray::updateSensorValues() {
  m_irMin = 1024;
  m_irMax = 0;
  for (int j = 0; j < IR_SENSOR_COUNT; j++) {
    m_irValue[j] = analogRead(m_config.recvPins[j]);
    m_irMin = min(m_irMin, m_irValue[j]);
    m_irMax = max(m_irMax, m_irValue[j]);
  }

  m_irAvg = 0;
  for (int val : m_irValue) {
    m_irAvg += val;
  }
  m_irAvg /= IR_SENSOR_COUNT;


  double sum = 0;
  for (int val : m_irValue) {
    sum += sq(double(val - m_irAvg));
  }
  m_irStdDev = sqrt(sum / IR_SENSOR_COUNT);
}
