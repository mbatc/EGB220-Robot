// #include "MotorDriver.h"
// #include "arduino.h"
// #include "avr/io.h"

// MotorDriver::MotorDriver(char reg, int pin, float speed, bool start)
//     : m_pin(pin)
// {
//   switch (reg)
//   {
//     case 'b':
//     case 'd':
//   }

//   PORTD &= 0;

//   SetSpeed(speed);
//   if (start)
//     Start();
// }

// void MotorDriver::Start()
// {
//   m_isActive = true;
//   ApplyPinState();
// }

// void MotorDriver::Stop()
// {
//   m_isActive = false;
//   ApplyPinState();
// }

// void MotorDriver::SetSpeed(float speed)
// {
//   // Clamp between 0 and 1
//   m_speed = speed < 0 ? 0 : speed > 1 ? 1 : speed;
//   ApplyPinState();
// }

// bool MotorDriver::IsActive() const
// {
//   return m_isActive;
// }

// float MotorDriver::GetSpeed() const
// {
//   return m_speed;
// }


// void MotorDriver::ApplyPinState()
// {
//   analogWrite(m_pin, int(m_speed * 255));
// }