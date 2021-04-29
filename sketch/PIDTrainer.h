#ifndef PIDTrainer_h__
#define PIDTrainer_h__

#include "Util.h"

class PIDController;
class SensorArray;

class PIDTrainer
{
public:
  enum
  {
    kP,
    kI,
    kD,
    kCount,
  };
  
  struct Parameters
  {
    double coeff[kCount] = { 0 };
    double cost = FLT_MAX;
    unsigned int start = 0;
  };

  PIDTrainer(PIDController *pController);

  void setStepSize(int paramID, double stepSize);

  // Update the trainer
  void update();

  void begin();

  void applyBest();

  double end();

protected:
  Parameters m_currentModel;
  Parameters m_bestModel;
  Parameters m_lastModel;
  
  int m_curParam = kP;

  int    m_direction[kCount] = { 0 };
  double m_stepSize[kCount]  = { 0 };
  bool   m_isTraining = false;

  PIDController *m_pPID = nullptr;
  SensorArray *m_pArray = nullptr;
};

#endif
