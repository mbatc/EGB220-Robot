#include "SensorArray.h"

SensorArray sensorArray; // Sensor array object. Handles reading sensors and calculating line position

const int motors_R     = 3;        // Right motor pin
const int motors_L     = 11;       // Left motor pin
const int motorsDir[2] = {17, 60}; // Motor direction pins **** not pin 60**** just havent been bothered to find it yet

bool driving = false;    // Is the motor currently driving
int maxMotorSpeed = 200; // Max speed of the motors
int minMotorSpeed = 2;   // Max speed of the motors
double turnAmount = 0;   // Current amount the robot is turning. Contains a value between -1 and 1 (-1=right, 1=left)

// Controls the linearity of the motors change in speed. e.g. 2 means the motor
// speed is decreased according to the curve x^2, and 3 would follow x^3.
//
// Larger values increase the 'deadzone' in the middle of the sensor values
// and cause a later, more aggressive turn.
double motorTurnFalloff = 0.75;

// The maximum amount the speed will be decrease when turning.
// Increase this number to increase speed around corners.
// A value of 0 will not adjust the max motor speed when cornering.
double maxTurnSpeedDecrease = 0.5;

// How much to divide the speed of both motors by if no line is detected
// Increase to make the robot move slower when it fails to detect a line. 
// A value greater than 1 will decrease the robots speed.
// A value of 1 will not change the robots speed.
// A value less than 1 will increase the robots speed.
double noLineSpeedScalar = 1.2;

void setup() {
  // Enabling serial
  Serial.begin(9600);
  DEBUG_PRINT("Ready...");

  // Configuration for the IR sensor array
  SensorConfig sensorConf = {
    // IR Emitter Pin
    .emitPin = 14,

     // IR Sensor Pins
    .recvPins = { 15, 2, 1, 0, 8, 7, 29, 6 },

     // Minimum std deviation between sensor readings for a line to be detected
    .detectThreshold = 150,

     // Controls how heavily weighted larger sensor values are. A larger number
     // will increase there weighting. This should not be greater than 1.
    .sensorFalloff = 0.25
  };

  // Setup the sensor array
  sensorArray.setup(sensorConf);
  
  //setting pins as inputs or outputs depending on what they are
  //motors are outputs
  for (int i = 0; i < 2; i++) {
    pinMode(motors_R, OUTPUT);
    pinMode(motors_L, OUTPUT);
  }    
}

// This function takes a speed for each motor and sets their pins.
void applyMotorSpeed(int leftMotor, int rightMotor) {
  analogWrite(motors_L, leftMotor);
  analogWrite(motors_R, rightMotor);
}

// Funtion to drive the robot, takes a speed value between 0 and 255 (0% and 100%) as the top speed
// both motors run at MotorSpeed until it sees a corner, then one wheel is ramped down in speed based on how sharp the corner is. this can be edited to make one wheel slow and the other speed up if need be
// motorLowestSpeed set the lowest speed the motors are allowed to go
void drive(int motorSpeed, int motorLowestSpeed) {
  // Get the line position calculated by the sensor array
  double linePos = sensorArray.getLinePos();
  
  int motorSpeed_L = 0; // Spped of the left motor
  int motorSpeed_R = 0; // Speed of the right motor

  // If the sensor array has detected a line, update turnAmount
  if (sensorArray.lineDetected()) {
    turnAmount = mapf(linePos, 1, 6, -1, 1);
    turnAmount = min(1, max(-1, turnAmount)); // Clamp between -1 and 1.
  }

  // Total difference in speed allowed
  int speedRange = motorSpeed - motorLowestSpeed;

  // If turnAmount is > 0, we want to turn left. Decrease the left motor speed, and keep the right motor unchanged.
  double leftAmount  = max(turnAmount, 0);      // Applied to the left motor
  // If turnAmount is < 0, we want to turn right. Decrease the right motor speed and keep the left motor unchanged.
  double rightAmount = abs(min(turnAmount, 0)); // Applied to the right motor
  double turnMagnitude = abs(turnAmount);
  
  // Map turn amounts to a non-linear curve.
  rightAmount    = pow(rightAmount,   motorTurnFalloff);
  leftAmount     = pow(leftAmount,    motorTurnFalloff);
  turnMagnitude  = pow(turnMagnitude, motorTurnFalloff);

  // Turn amount in either direction (used to calculate an adjustment for both motors).
  double motorSpeedFactor = (1 - mapf(turnMagnitude, 0, 1, 0, maxTurnSpeedDecrease)); // Applied to both motors.

  // Calculate the speed for each motor from the values above.
  motorSpeed_R = motorSpeedFactor * (motorSpeed - rightAmount * speedRange);
  motorSpeed_L = motorSpeedFactor * (motorSpeed - leftAmount  * speedRange);

  // Reduce the motor speed if no line is detected
  if (!sensorArray.lineDetected() && !sensorArray.horizontalLineDetected()) {
    motorSpeed_R /= noLineSpeedScalar;
    motorSpeed_L /= noLineSpeedScalar;
  }

  // Clamp calculated speeds between the min/max speeds given to the function
  motorSpeed_R = min(motorSpeed, max(motorLowestSpeed, motorSpeed_R));
  motorSpeed_L = min(motorSpeed, max(motorLowestSpeed, motorSpeed_L));
  
  // Updating the motors with their new speeds
  applyMotorSpeed(motorSpeed_L, motorSpeed_R);

  // More debugging serial prints
  DEBUG_PRINT("     Right motor speed: ");
  DEBUG_PRINT(motorSpeed_R);
  DEBUG_PRINT("     Left motor speed: ");
  DEBUG_PRINT(motorSpeed_L);
  
  DEBUG_PRINT("     Turn amount: ");
  DEBUG_PRINT(turnMagnitude * SIGN(turnAmount));
}

void loop() {
  // Am having a weird issue where the left motor direction reverts to backwards even though I've set it to forwards
  // Setting to forwards every loop seems to fix it for now though
  pinMode(17, OUTPUT);
  digitalWrite(17, LOW);

  // Update sensor array and calculate line position
  sensorArray.update();

  if (driving) {
    // Drive the robot
    drive(maxMotorSpeed, minMotorSpeed);

    // Keep driving while the line has not been missing for more than 0.5 seconds
    driving &= sensorArray.lineMissingTime() < 500;
  }
  else {
    // Make sure both motors are stopped
    applyMotorSpeed(0, 0);
    
    // Don't start driving until the line has been detected for more than 1 second.
    driving |= sensorArray.lineDetectedTime() > 1000;
  }

  // Print a new line on serial so it looks nice
  DEBUG_PRINTLN("");
}
