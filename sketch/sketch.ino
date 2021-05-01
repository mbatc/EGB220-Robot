#include "SensorArray.h"
#include "SerialCommands.h"
#include "Commands.h"
#include "StringStream.h"
#include "SoftwareSerial.h"
#include "PIDController.h"
#include "PIDTrainer.h"
#include "Bluetooth.h"

SensorArray sensorArray; // Sensor array object. Handles reading sensors and calculating line position

const int motors_R     = 3;          // Right motor pin
const int motors_L     = 11;         // Left motor pin
const int motorsDir[2] = { 17, 60 }; // Motor direction pins **** not pin 60**** just havent been bothered to find it yet

bool driving = false;    // Is the motor currently driving
int maxMotorSpeed = 170; // Max speed of the motors
int minMotorSpeed = 0;   // Max speed of the motors
int avgCount = 1;
double turnAmount = 0;   // Current amount the robot is turning. Contains a value between -1 and 1 (-1=right, 1=left)
double speedFactor = 0;
int useRawLinePos = 0;
int training = 0;
bool shouldSendParams = false;

double kp = 1;
double ki = 0;
double kd = 5;
double PIDScaleFactor = 1; // this value adjusts how sensitive the PID correction is on the turning
double avgCorrection = 0;
double avgSmoothing = 2;
int correctionSamples = 5;

float correction = 0;
PIDController pidController;
PIDTrainer    pidTrainer(&pidController);

void nextIteration()
{
  if (training)
  {
    pidTrainer.end();
    pidTrainer.begin();
  }
}

// Expose variables to the command interface
Commands::VarDef cmdVars[] = {
  { "kp", kp },
  { "ki", ki },
  { "kd", kd },
  { "PIDscaleFactor", PIDScaleFactor },
  { "maxMotorSpeed",  maxMotorSpeed  },
  { "minMotorSpeed",  minMotorSpeed  },
  { "useRawLinePos",  useRawLinePos  },
  { "avgCount", avgCount },
  { "training", training },
};

// Expose functions to the command interface
Commands::CmdDef cmdList[] = {
  { "nextIteration", nextIteration }
};

// Create the command set
Commands cmdSet(cmdList, ArraySize(cmdList), cmdVars, ArraySize(cmdVars));

Bluetooth bt(10, 9, &cmdSet);

void setup() {
  // Enabling serial
  Serial.begin(9600);

  // Configuration for the IR sensor array
  SensorConfig sensorConf = {
    // IR Emitter Pin
    .emitPin = 14,

    // IR Sensor Pins
    // .recvPins = { 15, 2, 1, 0, 8, 7, 29, 6 },
    .recvPins = { 2, 1, 0, 8, 7, 29 },

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
  
  pidController.setTarget(0.5);
}

// This function takes a speed for each motor and sets their pins.
void applyMotorSpeed(int leftMotor, int rightMotor) {
  analogWrite(motors_L, leftMotor);
  analogWrite(motors_R, rightMotor);
}

int motorSpeed_L = 0; // Speed of the left motor
int motorSpeed_R = 0; // Speed of the right motor

// Funtion to drive the robot, takes a speed value between 0 and 255 (0% and 100%) as the top speed
// both motors run at MotorSpeed until it sees a corner, then one wheel is ramped down in speed based on how sharp the corner is. this can be edited to make one wheel slow and the other speed up if need be
// motorLowestSpeed set the lowest speed the motors are allowed to go
void drive(int motorSpeed, int motorLowestSpeed) {
   // Get the line position calculated by the sensor array
  double linePos = useRawLinePos ? sensorArray.getLinePosRaw() : sensorArray.getLinePos();
  sensorArray.setAverageSampleCount(avgCount);
  
  // de-bugging serial print, can be comented out
  // DEBUG_PRINT("     Line Pos: ");
  // DEBUG_PRINT(linePos);

  // Update the PID controller if a line is detected
  if (sensorArray.lineDetected()) {
    correction = pidController.addSample(linePos, millis()) * (motorSpeed - motorLowestSpeed) * PIDScaleFactor;

    // Calculate an exponential moving average for the correction value.
    // This is used to adjust the 
    expMovingAverage(&avgCorrection, correction, correctionSamples, avgSmoothing);
    
    pidTrainer.update();

    if (correction > 0){
      motorSpeed_R = motorSpeed - correction;
      motorSpeed_L = motorSpeed;
    }
    else if (correction < 0){
      motorSpeed_L = motorSpeed + correction;
      motorSpeed_R = motorSpeed;
    }
  
    int cornerAdjustment = speedFactor * avgCorrection;
    motorSpeed_L -= cornerAdjustment;
    motorSpeed_R -= cornerAdjustment;
          
    // Clamp calculated speeds between the min/max speeds given to the function
    motorSpeed_R = min(motorSpeed, max(motorLowestSpeed, motorSpeed_R));
    motorSpeed_L = min(motorSpeed, max(motorLowestSpeed, motorSpeed_L));
  }
  
  DEBUG_PRINT("     Correction: ");
  DEBUG_PRINT(correction);

  // Updating the motors with their new speeds
  applyMotorSpeed(motorSpeed_L, motorSpeed_R);

  // More debugging serial prints
  // DEBUG_PRINT("     RMS: ");
  // DEBUG_PRINT(motorSpeed_R);
  // DEBUG_PRINT("     LMS: ");
  // DEBUG_PRINT(motorSpeed_L);
}

void loop() {
  // Update the bluetooth connection
  // reads/writes data from the module and processes commands
  bt.update();

  // Am having a weird issue where the left motor direction reverts to backwards even though I've set it to forwards
  // Setting to forwards every loop seems to fix it for now though
  pinMode(17, OUTPUT);
  digitalWrite(17, LOW);

  // Update sensor array and calculate line position
  sensorArray.update();

  if (driving) {
    if (training)
    {
      pidTrainer.begin();
      kp = pidController.getP();
      ki = pidController.getI();
      kd = pidController.getD();
    }
    else
    {
      pidController.setP(kp);
      pidController.setI(ki);
      pidController.setD(kd);
    }

    // Drive the robot
    drive(maxMotorSpeed, minMotorSpeed);

    // Keep driving while the line has not been missing for more than 0.1 seconds
    driving &= sensorArray.lineMissingTime() < 100;
  }
  else {
    // Make sure both motors are stopped
    applyMotorSpeed(0, 0);

    // Reset accum error when we loose the line
    pidController.reset();
    
    // Don't start driving until the line has been detected for more than 1 second.
    driving |= sensorArray.lineDetectedTime() > 1000;

    pidTrainer.end();
  }

  // Print a new line on serial so it looks nice
  DEBUG_PRINTLN("");
}
