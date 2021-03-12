// #ifndef MotorDriver_h__
// #define MotorDriver_h__

// class MotorDriver
// {
// public:
//   MotorDriver(char reg, int pin, float speed = 1, bool start = false);

//   /**
//    * Start the motor.
//    **/
//   void Start();

//   /**
//    * Stop the motor. 
//    **/
//   void Stop();

//   /**
//    * Returns true if the motor is being driven.
//    **/
//   bool IsActive() const;

//   /**
//    * Set the speed which the motor is spinning at.
//    * This is not the physical speed, but a fraction between 0 and 1. 
//    **/
//   void SetSpeed(float speed);

//   /**
//    * Get the speed which the motor is spinning at.
//    **/
//   float GetSpeed() const;

// protected:
//   /**
//    * This function is used to apply the motor drivers current state to the output pin
//    **/
//   void ApplyPinState();

//   bool  m_isActive = false;
//   float m_speed    = 1;
//   int   m_pin      = -1;
// };

// #endif