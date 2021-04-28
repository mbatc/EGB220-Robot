#include "SensorArray.h"
#include "SerialCommands.h"
#include "Commands.h"
#include "StringStream.h"
#include "SoftwareSerial.h"

SensorArray sensorArray; // Sensor array object. Handles reading sensors and calculating line position

const int motors_R     = 3;          // Right motor pin
const int motors_L     = 11;         // Left motor pin
const int motorsDir[2] = { 17, 60 }; // Motor direction pins **** not pin 60**** just havent been bothered to find it yet

bool driving = false;    // Is the motor currently driving
int maxMotorSpeed = 170; // Max speed of the motors
int minMotorSpeed = 0;   // Max speed of the motors
double turnAmount = 0;   // Current amount the robot is turning. Contains a value between -1 and 1 (-1=right, 1=left)
double speedFactor = 0;
bool useRawLinePos = false;
double kp = 1;
double ki = 0;
double kd = 5;
int PIDscaleFactor = 150; // this value adjusts how sensitive the PID correction is on the turning

// Expose variables to the command interface
Commands::VarDef cmdVars[] = {
  { "kp", kp },
  { "ki", ki },
  { "kd", kd },
  { "PIDscaleFactor", PIDscaleFactor },
  { "maxMotorSpeed",  maxMotorSpeed  },
  { "minMotorSpeed",  minMotorSpeed  },
  { "useRawLinePos",  useRawLinePos  },
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

unsigned long currentTime, previousTime;
double errorPID = 0;
double lastError = 0;
double correction = 0;
double cumError = 0;
double rateError = 0;
double idealLinePos = 3.5; // the ideal position of the robot on the line
bool isFirst = true;

int motorSpeed_L = 0; // Speed of the left motor
int motorSpeed_R = 0; // Speed of the right motor

// Funtion to drive the robot, takes a speed value between 0 and 255 (0% and 100%) as the top speed
// both motors run at MotorSpeed until it sees a corner, then one wheel is ramped down in speed based on how sharp the corner is. this can be edited to make one wheel slow and the other speed up if need be
// motorLowestSpeed set the lowest speed the motors are allowed to go
void drive(int motorSpeed, int motorLowestSpeed) {
   // Get the line position calculated by the sensor array
  double linePos = useRawLinePos ? sensorArray.getLinePosRaw() : sensorArray.getLinePos();
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
  
  //DEBUG_PRINT("     Perror: ");
  //DEBUG_PRINT(errorPID);
  //DEBUG_PRINT("     Ierror: ");
  //DEBUG_PRINT(cumError);
  //DEBUG_PRINT("     Derror: ");
  //DEBUG_PRINT(rateError);
  DEBUG_PRINT("     Correction: ");
  DEBUG_PRINT(correction);
      
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

  if (driving) {
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
