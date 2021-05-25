#include "SensorArray.h"
#include "SerialCommands.h"
#include "Commands.h"
#include "StringStream.h"
#include "SoftwareSerial.h"
#include "PIDController.h"
#include "Bluetooth.h"
#include "ColourSensor.h"
#include "List.h"
#include "TrackMap.h"

SensorArray   sensorArray; // Sensor array object. Handles reading sensors and calculating line position
PIDController pidController;
ColourSensor  colourSensor;
TrackMap      trackMap;

const int motors_R     = 3;          // Right motor pin
const int motors_L     = 11;         // Left motor pin
const int motorsDir[2] = { 17, 60 }; // Motor direction pins **** not pin 60**** just havent been bothered to find it yet

bool   driving        = false;    // Is the motor currently driving

int    maxMotorSpeed  = 170; // Max speed of the motors
int    minMotorSpeed  = 0;   // Max speed of the motors
int    curMotorSpeed  = maxMotorSpeed;
int    acceleration   = 5;
int    slowSpeed      = maxMotorSpeed / 2;

double turnAmount     = 0;   // Current amount the robot is turning. Contains a value between -1 and 1 (-1=right, 1=left)

double kp = 1;
double ki = 0;
double kd = 5;
double PIDScaleFactor = 1; // this value adjusts how sensitive the PID correction is on the turning

double avgSpeedDiff = 0;
double avgSmoothing = 2;
int speedDiffSamples = 5;
double changeThreshold = 0.05;
double speedUpThreshold = 0.2;

float correction = 0;

// Expose variables to the command interface
Commands::VarDef cmdVars[] = {
  { "kp", kp },
  { "ki", ki },
  { "kd", kd },
  { "PIDscaleFactor", PIDScaleFactor },
  { "maxMotorSpeed",  maxMotorSpeed  },
  { "minMotorSpeed",  minMotorSpeed  },
  { "changeThreshold", changeThreshold },
  { "acceleration", acceleration },
  { "slowSpeed", slowSpeed },
  { "speedUpThreshold", speedUpThreshold }
};

// Expose functions to the command interface
Commands::CmdDef cmdList[] = {
};

// Create the command set
Commands cmdSet(cmdList, ArraySize(cmdList), cmdVars, ArraySize(cmdVars));

Bluetooth bt(10, 9, &cmdSet);

void setup() {
  // Enabling serial
  Serial.begin(9600);

  // Configuration for the IR sensor array
  SensorConfig sensorConf = {
    // Marker sensor pins
    .leftMarkerPin = 22,
    .rightMarkerPin = 23,
    
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

long segmentMotorDiff = 0;
int  segmentLoops     = 0;

void onStartDriving();
void onStopDriving();
void onLapStart();
void startSegment();
void finishSegment();
void onLapEnd();
void onLineDetected();
void onMarkerDetected(MarkerSensor sensorID);
void onColourDetected(Colour colour);

// Funtion to drive the robot, takes a speed value between 0 and 255 (0% and 100%) as the top speed
// both motors run at MotorSpeed until it sees a corner, then one wheel is ramped down in speed based on how sharp the corner is. this can be edited to make one wheel slow and the other speed up if need be
// motorLowestSpeed set the lowest speed the motors are allowed to go
void drive() {
   // Get the line position calculated by the sensor array
  double linePos = sensorArray.getLinePos();
  
  debugPrint("D Factor", pidController.getRateOfChange());

  if (sensorArray.isMarkerDetected(MS_Left)) {
    onMarkerDetected(MS_Left);
  }
  else if (sensorArray.isMarkerDetected(MS_Right)) {
    onMarkerDetected(MS_Right);
  }

  if (abs(pidController.getRateOfChange()) > changeThreshold) {
    curMotorSpeed = slowSpeed;
  }

  // Update the PID controller if a line is detected
  if (sensorArray.lineDetected()) {
    onLineDetected(linePos);
  }
  
  debugPrint("motorSpeed", curMotorSpeed);
  debugPrint("Correction", correction);
  
  debugPrint("LMS", motorSpeed_L);
  debugPrint("RMS", motorSpeed_R);

  // Updating the motors with their new speeds
  applyMotorSpeed(motorSpeed_L, motorSpeed_R);
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
    pidController.setP(kp);
    pidController.setI(ki);
    pidController.setD(kd);

    // Drive the robot
    drive();

    // Keep driving while the line has not been missing for more than 0.1 seconds
    driving &= sensorArray.lineMissingTime() < 100;
    
    if (!driving) {
      onStopDriving();
    }
  }
  else {
    // Make sure both motors are stopped
    applyMotorSpeed(0, 0);

    // Reset accum error when we loose the line
    pidController.reset();
    
    // Don't start driving until the line has been detected for more than 1 second.
    driving |= sensorArray.lineDetectedTime() > 1000;

    if (driving) {
      onStartDriving();
    }
  }

  // Print a new line on serial so it looks nice
  DEBUG_PRINTLN("");
}

// When we start driving we want to set reset the PID control system and set the colour sensor to the correct colour
void onStartDriving() {
  colourSensor.SetTarget(Col_Green);
}

void onStopDriving() {

}

// When a line is detected, we want to update the PID control system and calculate new motor speeds
void onLineDetected(double linePos) {
  correction = pidController.addSample(linePos, millis()) * (curMotorSpeed - minMotorSpeed) * PIDScaleFactor;

  if (correction > 0) {
    motorSpeed_R = curMotorSpeed - correction;
    motorSpeed_L = curMotorSpeed;
  }
  else if (correction < 0) {
    motorSpeed_L = curMotorSpeed + correction;
    motorSpeed_R = curMotorSpeed;
  }

  // Clamp calculated speeds between the min/max speeds given to the function
  motorSpeed_R = min(maxMotorSpeed, max(minMotorSpeed, motorSpeed_R));
  motorSpeed_L = min(maxMotorSpeed, max(minMotorSpeed, motorSpeed_L));

  int speedDiff = motorSpeed_L - motorSpeed_R;

  // Keep track of the motor speed difference for the track map
  segmentMotorDiff += speedDiff;
  ++segmentLoops;

  // Calculate an exponential moving average for the correction value.
  expMovingAverage(&avgSpeedDiff, speedDiff, speedDiffSamples, avgSmoothing);

  float diffFactor = abs(avgSpeedDiff) / (float)curMotorSpeed;
  debugPrint("avg Diff", avgSpeedDiff);
  if (diffFactor < speedUpThreshold)
  {
    curMotorSpeed += acceleration;
    curMotorSpeed = min(curMotorSpeed, maxMotorSpeed);
  }
}

void onMarkerDetected(MarkerSensor sensorID) {
  if (sensorID == MS_Right) {
    if (colourSensor.IsDetected()) {
      onColourDetected(colourSensor.GetTarget());
    }
  }

  finishSegment();
}

void onColourDetected(Colour col) {
  switch (col)
  {
  case Col_Green:
    onLapStart();
    colourSensor.SetTarget(Col_Red);
    break;
  case Col_Red:
    onLapEnd();
    colourSensor.SetTarget(Col_Green);
    break;
  }
}

void onLapStart() {

}

void onLapEnd() {

}

unsigned long segmentStartTime = 0;

void startSegment() {
  segmentMotorDiff = 0;
  segmentStartTime = millis();
}

void finishSegment() {
  // Find the motor speed diff, (/2 so we can guarantee the result will fit in a int8)
  int8_t motorDiff     = (segmentMotorDiff / segmentLoops) / 2;
  unsigned long length = (millis() - segmentStartTime) / 50;
  uint8_t length8      = uint8_t(min(length, 255));

  trackMap.addSection(segmentMotorDiff / segmentLoops, length);
  
  bt.write(0x0F);
  bt.write(uint8_t(128 + motorDiff)); // re-interpret as a uint8_t
  bt.write(length8);
  bt.write('\0');
}
