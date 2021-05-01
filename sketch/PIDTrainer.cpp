#include "PIDController.h"
#include "PIDTrainer.h"
#include "Util.h"
#include "Bluetooth.h"

PIDTrainer::PIDTrainer(PIDController *pController)
{
  for (int &dir : m_direction)
    dir = 1;
  m_pPID   = pController;

  m_stepSize[kP] = 0.1;
  m_stepSize[kI] = 0.0001;
  m_stepSize[kD] = 1;
}

void PIDTrainer::setStepSize(int paramID, double stepSize)
{
  m_stepSize[paramID] = stepSize;
}

void PIDTrainer::update()
{
  if (m_isTraining)
  {
    m_currentModel.cost += sq(m_pPID->getError());
    DEBUG_PRINT("    Training Model: ");
    DEBUG_PRINT(1000 * m_currentModel.cost / (millis() - m_currentModel.start));
    
    DEBUG_PRINT("    PID: ");
    DEBUG_PRINT(m_currentModel.coeff[kP]);
    DEBUG_PRINT("  ");
    DEBUG_PRINT(m_currentModel.coeff[kI]);
    DEBUG_PRINT("  ");
    DEBUG_PRINT(m_currentModel.coeff[kD]);
  }
}

void PIDTrainer::begin()
{
  if (m_isTraining)
    return;
  m_currentModel.coeff[kP] = m_pPID->getP();
  m_currentModel.coeff[kI] = m_pPID->getI();
  m_currentModel.coeff[kD] = m_pPID->getD();
  m_currentModel.cost = 0;
  m_currentModel.start = millis();

  bt.print("New Training Iteration (PID: ");
  bt.print(m_pPID->getP(), 7);
  bt.print("  ");
  bt.print(m_pPID->getI(), 7);
  bt.print("  ");
  bt.print(m_pPID->getD(), 7);
  bt.print(")");
  m_isTraining = true;
}

double PIDTrainer::end()
{
  if (!m_isTraining)
    return;

  m_isTraining = false;

  // Scale the cost based on how long we were recording the error for
  m_currentModel.cost /=  millis() - m_currentModel.start;
  bt.print("Finished Interation (Cost: ");
  bt.print(m_currentModel.cost, 7);
  bt.print(")");

  // Check if the current model is the best, if it is, keep track of it
  if (m_currentModel.cost < m_bestModel.cost)
    m_bestModel = m_currentModel;

  // This model is worse than the previous.
  // Go back to the previous and change the step direction for the parameter we just updated
  if (m_lastModel.cost < m_currentModel.cost)
  {
    m_direction[m_curParam] = -SIGN(m_direction[m_curParam]);
    m_currentModel = m_lastModel;
  }

  int nextParam = (m_curParam + 1) & kCount;
  while (m_stepSize[nextParam] == 0 && nextParam != m_curParam)
    nextParam = (nextParam + 1) % kCount;
  m_curParam = nextParam;
  
  // Adjust the next parameter
  m_currentModel.coeff[m_curParam] += m_stepSize[m_curParam] * m_direction[m_curParam];

  // Update PID parameters
  m_pPID->setP(m_currentModel.coeff[kP]);
  m_pPID->setI(m_currentModel.coeff[kI]);
  m_pPID->setD(m_currentModel.coeff[kD]);

  // Return the cost of the current model
  return m_currentModel.cost;
}
