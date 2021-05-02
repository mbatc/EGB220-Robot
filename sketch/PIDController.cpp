#include "PIDController.h"
#include "Util.h"
 
float PIDController::addSample(float sample, int currentTime)
{
  float error = m_setPoint - sample;
  
  // Get the time since the last sample
  int elapsedTime = max(currentTime - m_lastTime, 1);

  // Calculate integral and derivative factors
  float rateError = 0.0f;
  if (!m_isFirst) {
    m_accum += error * elapsedTime; 
    m_rateOfChange = (error - m_lastError) / elapsedTime;
  }  
  
  // Update controller state
  m_isFirst   = false;
  m_lastError = error;
  m_lastTime  = currentTime;

  // Calculate the correction value
  return m_kp * error + m_ki * m_accum + m_rateOfChange * m_kd;
}

float PIDController::getRateOfChange() const { return m_rateOfChange * 1000; }

void PIDController::reset()
{
  m_isFirst = true;
  m_accum = 0;
  m_lastError = 0;
  m_lastTime = millis();
}

float PIDController::getError() const { return m_lastError; }

void PIDController::setTarget(float setPoint) { m_setPoint = setPoint; }

void PIDController::setP(float val) { m_kp = val; }
void PIDController::setI(float val) { m_ki = val; }
void PIDController::setD(float val) { m_kd = val; }

float PIDController::getP() const { return m_kp; }
float PIDController::getI() const { return m_ki; }
float PIDController::getD() const { return m_kd; }
