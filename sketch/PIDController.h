#ifndef PIDController_h__
#define PIDController_h__

class PIDController
{
public:
  // Add a sample to the PID controller.
  // Returns a correction value to be applied to the system
  float addSample(float error, int currentTime);
  void setTarget(float setPoint);
  
  float getError() const;
  
  void reset();

  // Set the controllers parameters
  void setP(float val);
  void setI(float val);
  void setD(float val);

  // Get the controllers parameters
  float getP() const;
  float getI() const;
  float getD() const;
  
protected:
  float m_kp = 1;
  float m_ki = 0;
  float m_kd = 1;
  float m_setPoint = 0.0f;
  
  float m_accum     = 0;
  float m_lastError = 0;
  int m_lastTime    = 0;
  bool m_isFirst    = false;
};

#endif
