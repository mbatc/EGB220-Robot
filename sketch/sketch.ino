#include "SensorArray.h"
#include "SerialCommands.h"
#include "Commands.h"
#include "StringStream.h"
#include "SoftwareSerial.h"

SensorArray sensorArray; // Sensor array object. Handles reading sensors and calculating line position

const int motors_R     = 3;        // Right motor pin
const int motors_L     = 11;       // Left motor pin
const int motorsDir[2] = {17, 60}; // Motor direction pins **** not pin 60**** just havent been bothered to find it yet

bool driving = false;    // Is the motor currently driving
int maxMotorSpeed = 170; // Max speed of the motors
int minMotorSpeed = 0;   // Max speed of the motors
double turnAmount = 0;   // Current amount the robot is turning. Contains a value between -1 and 1 (-1=right, 1=left)
double speedFactor = 0;
int calibrate = 0;

double kp = 1;
double ki = 0;
double kd = 5;
double PIDscaleFactor = 0.75; // this value adjusts how sensitive the PID correction is on the turning

int mode = 0;

// Controls the linearity of the motors change in speed. e.g. 2 means the motor
// speed is decreased according to the curve x^2, and 3 would follow x^3.
//
// Larger values increase the 'deadzone' in the middle of the sensor values
// and cause a later, more aggressive turn.
double motorTurnFalloff = 0.75;

// The maximum amount the speed will be decreased when turning.
// Decrease this number to increase speed around corners.
// A value of 0 will not adjust the max motor speed when cornering.
double maxTurnSpeedDecrease = 0.5;

// How much to divide the speed of both motors by if no line is detected
// Increase to make the robot move slower when it fails to detect a line.
// A value greater than 1 will decrease the robots speed.
// A value of 1 will not change the robots speed.
// A value less than 1 will increase the robots speed.
double noLineSpeedScalar = 1.2;

// Expose variables to the command interface
Commands::VarDef cmdVars[] = {
  { "kp",     kp },
  { "ki",     ki },
  { "kd",     kd },
  { "PIDscaleFactor", PIDscaleFactor },
  { "maxMotorSpeed", maxMotorSpeed },
  { "minMotorSpeed", minMotorSpeed },
  { "speedFactor", speedFactor },
  { "calibrate", calibrate }
};

// Expose functions to the command interface
Commands::CmdDef cmdList[] = {};

// Create the serial interface for the command set
StringStream commandBuffer;
bool commandReady = false;
SoftwareSerial bt(10, 9);

// Create the command set
Commands cmdSet(cmdList, ArraySize(cmdList), cmdVars, ArraySize(cmdVars));
SerialCommands serialCmd(&cmdSet, &commandBuffer, &bt);

void setup() {
  // Enabling serial
  Serial.begin(9600);
  bt.begin(9600);
 //DEBUG_PRINT("Ready...");

  // Configuration for the IR sensor array
  SensorConfig sensorConf = {
    // IR Emitter Pin
    .emitPin = 14,

    // IR Sensor Pins
    // .recvPins = { 15, 2, 1, 0, 8, 7, 29, 6 }, /*For 8 sensors*/
    .recvPins = { 2, 1, 0, 8, 7, 29 }, /*For 6 sensors*/

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

unsigned long currentTime, previousTime;
double errorPID = 0;
double lastError = 0;
double correction = 0;
double cumError = 0;
double rateError = 0;
double idealLinePos = 0.5; // the ideal position of the robot on the line
bool isFirst = true;

int motorSpeed_L = 0; // Speed of the left motor
int motorSpeed_R = 0; // Speed of the right motor

// Funtion to drive the robot, takes a speed value between 0 and 255 (0% and 100%) as the top speed
// both motors run at MotorSpeed until it sees a corner, then one wheel is ramped down in speed based on how sharp the corner is. this can be edited to make one wheel slow and the other speed up if need be
// motorLowestSpeed set the lowest speed the motors are allowed to go
void drive(int motorSpeed, int motorLowestSpeed) {
   // Get the line position calculated by the sensor array
  double linePos = sensorArray.getLinePos();

  currentTime = millis();
  double elapsedTime = (double)currentTime - previousTime;

  // Update the PID controller if a line is detected
  if (sensorArray.lineDetected()) {
    errorPID  = idealLinePos - linePos;
    if (!isFirst) {
      cumError += errorPID * elapsedTime; 
      rateError = (errorPID - lastError)/elapsedTime;
    }
    isFirst = false;
    correction = kp * errorPID + ki * cumError + kd * rateError;
    lastError = errorPID;

    correction = PIDscaleFactor * correction * (motorSpeed - motorLowestSpeed);
    
    if (correction > 0){
      motorSpeed_R = motorSpeed - correction;
      motorSpeed_L = motorSpeed;
    }
    else if (correction < 0){
      motorSpeed_L = motorSpeed + correction;
      motorSpeed_R = motorSpeed;
    }

    int cornerAdjustment = abs(motorSpeed_L - motorSpeed_R) * speedFactor;
    motorSpeed_L -= cornerAdjustment;
    motorSpeed_R -= cornerAdjustment;
  }
  
  previousTime = currentTime;
  DEBUG_PRINT("     Pe: ");
  DEBUG_PRINT(errorPID);
  DEBUG_PRINT("     Ie: ");
  DEBUG_PRINT(cumError);
  DEBUG_PRINT("     De: ");
  DEBUG_PRINT(rateError);
  DEBUG_PRINT("     Cor: ");
  DEBUG_PRINT(correction);
      
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
  DEBUG_PRINT("     RMS: ");
  DEBUG_PRINT(motorSpeed_R);
  DEBUG_PRINT("     LMS: ");
  DEBUG_PRINT(motorSpeed_L);
}

void loop() {  
  if (commandReady) {
    DEBUG_PRINT("\n");
    DEBUG_PRINT("Executing Command: ");
    DEBUG_PRINT(commandBuffer.str());
    DEBUG_PRINT("\n");

    serialCmd.execute();
    commandBuffer.flush();
    commandReady = false;
  }
  
  if (bt.available()) {
    commandReady |= bt.peek() == '\n'; // New line signals the end of a command 
    commandBuffer.write((char)bt.read());
  }

  // Am having a weird issue where the left motor direction reverts to backwards even though I've set it to forwards
  // Setting to forwards every loop seems to fix it for now though
  pinMode(17, OUTPUT);
  digitalWrite(17, LOW);

  // Update sensor array and calculate line position
  sensorArray.update();
  sensorArray.setCalibrating(calibrate != 0);
  
  if (driving && !calibrate) {
    // Drive the robot
    drive(maxMotorSpeed, minMotorSpeed);

    // Keep driving while the line has not been missing for more than 0.5 seconds
    driving &= sensorArray.lineMissingTime() < 100;
  }
  else {
    // Make sure both motors are stopped
    applyMotorSpeed(0, 0);

    // Reset accum error when we loose the line
    cumError = 0;
    isFirst = true;
    
    // Don't start driving until the line has been detected for more than 1 second.
    driving |= sensorArray.lineDetectedTime() > 1000;
  }

  // Print a new line on serial so it looks nice
  DEBUG_PRINTLN("");
}
