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

bool   isColourMarker = false;
Colour nextColour     = Col_Green;

const int motors_R     = 3;          // Right motor pin
const int motors_L     = 11;         // Left motor pin
const int motorsDir[2] = { 17, 60 }; // Motor direction pins **** not pin 60**** just havent been bothered to find it yet

bool   driving        = false;    // Is the motor currently driving
int    fastSpeed      = 170;
int    slowSpeed      = 80;
int    curMotorSpeed  = slowSpeed;

int    acceleration   = 5;
double kp = 2.5;
double ki = 0;
double kd = 75;
double PIDScaleFactor = 1; // this value adjusts how sensitive the PID correction is on the turning

double avgSpeedDiff = 0;
double avgSmoothing = 2;
int speedDiffSamples = 5;
double speedUpThreshold = 0.2;
float correction = 0;

void startCalibration() {
  sensorArray.resetCalibration();
  g_calibrateSensors = true;
}

void endCalibration() {
  g_calibrateSensors = false;
}

void startDriving() {
  driving = true;
}

void stopDriving() {
  driving = false;
}

// Expose variables to the command interface
Commands::VarDef cmdVars[] = {
  { "kp", kp },
  { "ki", ki },
  { "kd", kd },
  { "PIDscaleFactor", PIDScaleFactor },
  { "fastSpeed",  fastSpeed },
  { "slowSpeed",  slowSpeed },
  { "speedUpThreshold", speedUpThreshold }
};

// Expose functions to the command interface
Commands::CmdDef cmdList[] = {
  { "startCalib", startCalibration },
  { "endCalib", endCalibration },
  { "drive", startDriving },
  { "stop", stopDriving }
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
    .leftMarkerPin = 23,
    .rightMarkerPin = 22,
    
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
void onLapEnd();

void onLineDetected();
void onMarkerDetected(MarkerSensor sensorID);
void onColourDetected(Colour colour);

void driveSlow()
{  
  curMotorSpeed = slowSpeed;
}

void driveFast()
{
  curMotorSpeed = fastSpeed;
}

void onMarkerDetected(MarkerSensor sensorID);
void onMarkerDetectEnd(MarkerSensor sensorID);
void onMarkerDetectStart(MarkerSensor sensorID);

void updateMarkerDetected(MarkerSensor sensorID)
{
  static bool lastMarkerDetected[MS_Count] = { 0 };
  bool &lastDetected = lastMarkerDetected[sensorID];
  bool detected = sensorArray.isMarkerDetected(sensorID);
  if (detected)
  {
    if (!lastDetected)
      onMarkerDetectStart(sensorID);
    onMarkerDetected(sensorID);
  }
  else
  {
    if (lastDetected)
      onMarkerDetectEnd(sensorID);
  }

  lastDetected = detected;
}

// Funtion to drive the robot, takes a speed value between 0 and 255 (0% and 100%) as the top speed
// both motors run at MotorSpeed until it sees a corner, then one wheel is ramped down in speed based on how sharp the corner is. this can be edited to make one wheel slow and the other speed up if need be
// motorLowestSpeed set the lowest speed the motors are allowed to go
void drive() {
   // Get the line position calculated by the sensor array
  double linePos = sensorArray.getLinePos();

  updateMarkerDetected(MS_Left);
  updateMarkerDetected(MS_Right);

  
  if (colourSensor.isRed()) {
    driveSlow();
  }

  if (colourSensor.isGreen()) {
    driveFast();
  }

  // Update the PID controller if a line is detected
  if (sensorArray.lineDetected()) {
    correction = pidController.addSample(linePos, millis()) * curMotorSpeed * PIDScaleFactor;

    if (correction > 0){
      motorSpeed_R = curMotorSpeed - correction;
      motorSpeed_L = curMotorSpeed;
    }
    else if (correction < 0){
      motorSpeed_L = curMotorSpeed + correction;
      motorSpeed_R = curMotorSpeed;
    }
          
    // Clamp calculated speeds between the min/max speeds given to the function
    motorSpeed_R = min(curMotorSpeed, max(0, motorSpeed_R));
    motorSpeed_L = min(curMotorSpeed, max(0, motorSpeed_L));
    
    // Calculate an exponential moving average for the correction value.
    expMovingAverage(&avgSpeedDiff, motorSpeed_L - motorSpeed_R, speedDiffSamples, avgSmoothing);

    float diffFactor = abs(avgSpeedDiff) / (float)curMotorSpeed;
    debugPrint("avg Diff", avgSpeedDiff);
    // if (diffFactor < speedUpThreshold) {
    //  curMotorSpeed += acceleration;
    //  curMotorSpeed = min(curMotorSpeed, maxMotorSpeed);
    // }
  }
  
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
  colourSensor.update();
  
  if (driving) {
    pidController.setP(kp);
    pidController.setI(ki);
    pidController.setD(kd);

    // Drive the robot
    drive();

    // Keep driving while the line has not been missing for more than 0.1 seconds
    // driving &= sensorArray.lineMissingTime() < 100;
    
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
    // driving |= sensorArray.lineDetectedTime() > 1000;

    if (driving) {
      onStartDriving();
    }
  }

  // Print a new line on serial so it looks nice
  DEBUG_PRINTLN("");
}

// When we start driving we want to set reset the PID control system and set the colour sensor to the correct colour
void onStartDriving() {
  
}

void onStopDriving() {
  
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

void onColourDetected(Colour col) {
}

void onMarkerDetected(MarkerSensor sensorID)
{
  if (sensorID == MS_Left && !isColourMarker) {
    if (colourSensor.isDetected(nextColour)) {
      onColourDetected(nextColour);
      isColourMarker = true;
    }
  }
}

static bool colourDetected = false;

void onMarkerDetectEnd(MarkerSensor sensorID)
{
  isColourMarker = false;   
}

void onMarkerDetectStart(MarkerSensor sensorID)
{ 
}
