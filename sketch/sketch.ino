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
#include "Interrupts.h"

SensorArray   sensorArray; // Sensor array object. Handles reading sensors and calculating line position
PIDController pidController;
ColourSensor  colourSensor;
TrackMap      trackMap;

bool   isColourMarker = false;
Colour nextColour     = Col_Green;

const int motors_R     = 3;          // Right motor pin
const int motors_L     = 11;         // Left motor pin
const int motorsDir[2] = { 17, 60 }; // Motor direction pins **** not pin 60**** just havent been bothered to find it yet

bool   driving        = false; // Is the motor currently driving
bool   allowDrive     = true;  // Can the robot start driving
int    straightSpeed  = 170;   // The speed for the straights
int    cornerSpeed    = 150;    // The speed for the corners
int    slowSpeed      = 60;    // The speed for the slow zone
bool   inStraight     = false; // Is the robot in a straight track section
bool   allowAccell    = false; // Can the robot accelerate
int    curMotorSpeed  = cornerSpeed; // Setup the initial motor speed to be the cornering speed
int    acceleration   = 10;    // How fast does the robot accelerate
int    lapStopTime    = 4000;  // How many milliseconds to stop for between laps
int    colDetectThreshold = 10; // How many consecutive times a colour should be detected before taking action
int    stopDelay      = 0;

bool lastMarkerDetected[MS_Count] = { 0 };

// Control system parameters
double kp = 2.5; // Proportional control
double ki = 0;   // Integral control
double kd = 75;  // Derivative control
double PIDScaleFactor = 4; // this value adjusts how sensitive the PID correction is on the turning

// Average wheel speed difference
double avgSpeedDiff = 0;       // Average difference in motor speed
double avgSmoothing = 2;       // Exponential average smoothing
int speedDiffSamples = 10;     // Number of samples to include in the average 
double speedUpThreshold = 0.5; // The percentage motor difference to threshold to consider the track straight.
int straightLoops = 0;
// Current control system correction value
float correction = 0;

// Current lap details
bool lapStarted = false;
unsigned long lapStartTime = 0;
unsigned long lapFinishTime = 0;
unsigned long stopTime = 0;

unsigned long colourDetectTime  = 0;
int           maxColourIntensity = 0;

void startCalibration() {
  Serial.println("Start Calibration");
  sensorArray.resetCalibration();
  g_calibrateSensors = true;
}

void endCalibration() {
  Serial.println("End Calibration");
  g_calibrateSensors = false;
}

void startDriving() {
  allowDrive = true;
}

void stopDriving() {
  allowDrive = false;
}

// Expose variables to the command interface
Commands::VarDef cmdVars[] = {
  { "P",        kp },
  { "I",        ki },
  { "D",        kd },
  { "PIDsf",    PIDScaleFactor },
  { "srtSpd",   straightSpeed },
  { "crnSpd",   cornerSpeed },
  { "slwSpd",   slowSpeed },
  { "acl",      acceleration },
  { "spdUpThr", speedUpThreshold },
  { "stopDelay", stopDelay }
};

// Expose functions to the command interface
Commands::CmdDef cmdList[] = {
  { "startCalib", startCalibration },
  { "endCalib",   endCalibration },
  { "drive",      startDriving },
  { "stop",       stopDriving }
};

// Create the command set
Commands cmdSet(cmdList, ArraySize(cmdList), cmdVars, ArraySize(cmdVars));

Bluetooth bt(10, 9, &cmdSet);

void detectMarkers();

void toggleLED()
{
  static bool enableLED = false;

  Serial.println("In Interrupt");
  
  if (enableLED) {
    pinMode(13, OUTPUT);
    digitalWrite(13, HIGH);
  }
  else {
    pinMode(13, OUTPUT);
    digitalWrite(13, LOW);
  }

  enableLED = !enableLED;
}

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

  trackMap.addSection(0, 10);
  trackMap.addSection(30, 10);
  trackMap.addSection(0, 10);
  trackMap.addSection(30, 10);
  trackMap.addSection(0, 10);
  trackMap.addSection(30, 10);
  trackMap.addSection(0, 10);
  trackMap.addSection(30, 10);
  
  Interrupts::attach(0, 500, detectMarkers);
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

void onStartDriving()
{
  Serial.println("Start Driving");

  // Reset the stop time.
  stopTime = 0;
  
  // Reset accum error when we loose the line.
  pidController.reset();

  // When we start driving, assume we are moving towards
  // the lap start marker
  lapStarted = false;

  // Set the motor speed to the corner speed
  curMotorSpeed = cornerSpeed;
}

void sendTrackInfo()
{
  Serial.println("Send Track");

  bt.print("newtrack");
  bt.update(); // Send data
  for (size_t i = 0; i < trackMap.sectionCount(); ++i) {
    bt.print("sec ");
    bt.print((int)trackMap.sectionType(i));
    bt.print(" ");
    bt.print((int)trackMap.sectionLength(i));
    bt.update(); // Send data
  }

  bt.print("lap ");
  bt.print((lapFinishTime - lapStartTime));
  bt.update(); // Send data
}

void reset() {
  lapStarted = false;
  stopTime   = 0;
  lastMarkerDetected[0] = false;
  lastMarkerDetected[1] = false;
}

void onLapStart()
{
  Serial.println("Start Lap");
  lapStartTime = millis();
}

void onLapEnd()
{
  lapFinishTime = millis();
  stopTime = lapFinishTime + stopDelay;  
  debugPrint("End Lap: ", lapFinishTime + stopDelay);
  Serial.println();
}

void onEnterSlowZone() {
  Serial.println("Start Slow Zone");
  inStraight    = true;
  allowAccell   = false;
  curMotorSpeed = slowSpeed;
}

void onExitSlowZone() {
  Serial.println("Exit Slow Zone");
  inStraight = true;
  allowAccell = true;
  curMotorSpeed = cornerSpeed;
}

void onEnterCorner() {
  inStraight = false;
  curMotorSpeed = cornerSpeed;
}

void onLapBreak() {
  Serial.println("Lap finished");
  // Stop motors
  applyMotorSpeed(0, 0);
  driving = false;

  // Time when the lap break started.
  // We need to delay at least 2 seconds here
  unsigned long startTime = millis();

  // Send the track info the the connected PC
  sendTrackInfo();
 
  // Calculate the remaining wait time.
  long delayTime = lapStopTime - (millis() - startTime);

  if (delayTime > 0)
    delay(delayTime);

  stopTime = 0;
  lapStarted = false;
}

// Funtion to drive the robot, takes a speed value between 0 and 255 (0% and 100%) as the top speed
// both motors run at MotorSpeed until it sees a corner, then one wheel is ramped down in speed based on how sharp the corner is. this can be edited to make one wheel slow and the other speed up if need be
// motorLowestSpeed set the lowest speed the motors are allowed to go
void drive() {
   // Get the line position calculated by the sensor array
  double linePos = sensorArray.getLinePos();

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
    
    // Calculate an exponential moving average for the motor speed difference.
    expMovingAverage(&avgSpeedDiff, motorSpeed_L - motorSpeed_R, speedDiffSamples, avgSmoothing);
    float diffFactor = abs(avgSpeedDiff) / (float)curMotorSpeed;

    if (diffFactor < speedUpThreshold)
      ++straightLoops;
    else
      straightLoops = 0;

    if (straightLoops > 25) {
      allowAccell = true;
      inStraight = true;
    }

    if (inStraight && allowAccell) {
      curMotorSpeed = straightSpeed;
    }
    
    // if (inStraight)
    //   onStraightSection();
    // else
    //  onCornerSection();

    // if (allowAccell) {
    //  curMotorSpeed += acceleration;
    // }
  }

  // Updating the motors with their new speeds
  applyMotorSpeed(motorSpeed_L, motorSpeed_R);
}

void detectMarkers();

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
  
  if (g_calibrateSensors)
  {
    reset();
    return;
  }

  if (stopTime != 0 && millis() >= stopTime) {
    onLapBreak();
  }

  if (driving) {
    // Apply control system parameters
    pidController.setP(kp);
    pidController.setI(ki);
    pidController.setD(kd);

    // Detect track markers
    detectMarkers();

    // Drive the robot
    drive();

    // Keep driving while the line has not been missing for more than 0.1 seconds
    driving &= sensorArray.lineMissingTime() < 100 && allowDrive;
  }
  else {
    // Make sure both motors are stopped
    applyMotorSpeed(0, 0);
    
    // Don't start driving until the line has been detected for more than 1 second.
    driving |= sensorArray.lineDetectedTime() > 1000 && allowDrive;

    reset();

    if (driving) {
      onStartDriving();
    }
  }
}

void onColourDetected(Colour col) {
  static int           maxIntensity = 0;
  static unsigned long changeTime   = 0;
  static const int     detectTime   = 75;
  static Colour        lastColour   = Col_None;
  static bool          allowDetect  = false;
  
  int intensity = colourSensor.getIntensity();
  
  if (!allowDetect) {
    maxIntensity = 0;
    if (colourSensor.isBlack()) {
      allowDetect = true;
      return;
    }
  }

  if (intensity > maxIntensity) {
    maxIntensity = intensity;
    changeTime = millis();
    lastColour = colourSensor.getColour();
  }

  if (millis() - changeTime > detectTime) {
    debugPrint("Colour Detected", lastColour);
    Serial.println();

    if (lastColour == Col_Red) {
      onExitSlowZone();
    }
    else if (lastColour == Col_Green) {
      onEnterSlowZone();
    }
    else if (lastColour == Col_White) {
      onEnterCorner();
    }
    
    allowDetect = false;
    maxIntensity = 0;
    lastColour = Col_None;
  }
}

bool isStartStopMarker = true;

void onMarkerDetected(MarkerSensor sensorID) {
  if (sensorID == MS_Right) {
    isStartStopMarker &= colourSensor.isBlack();
  }
}

void onMarkerDetectEnd(MarkerSensor sensorID)
{
  if (sensorID == MS_Right && isStartStopMarker)
  {
    if (lapStarted)
      onLapEnd();
    else
      onLapStart();
    
    // Flip in lap state
    lapStarted = !lapStarted;
  }
  
  isStartStopMarker = true;
}

void onMarkerDetectStart(MarkerSensor sensorID)
{
}

void updateMarkerDetected(MarkerSensor sensorID)
{
  bool &lastDetected = lastMarkerDetected[sensorID];
  bool detected = sensorArray.isMarkerDetected(sensorID);
  if (detected)
  { 
    if (!lastDetected) {
      onMarkerDetectStart(sensorID);
    }
    onMarkerDetected(sensorID);
  }
  else
  {
    if (lastDetected) {
      onMarkerDetectEnd(sensorID);
    }
  }

  lastDetected = detected;
}

void detectMarkers()
{ 
  colourSensor.update();
  sensorArray.updateMarkers();

  onColourDetected(colourSensor.getColour());
  // updateMarkerDetected(MS_Left);
  updateMarkerDetected(MS_Right);
}
