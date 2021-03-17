#define PRINT_DEBUG_INFO

#include "math.h"
#include "SensorArray.h"

SensorArray sensorArray;

const int motors_R = 3; // Right motor
const int motors_L = 11; // Left motor

const int motorsDir[2] = {17, 60}; // motor direction pins **** not pin 60**** just havent been bothered to find it yet

double sensorPower  = 0.5;

int motorSpeed = 200; // Current speed of the motors

double calcPower(double x, double power, double shift, double scale) {
  return pow((x - shift)/scale, power) * scale;
}

double mapNonLinear(double input, double inMin, double inMax, double outMin, double outMax, double power) {
  double inRange  = inMax - inMin; // Range of the input value
  double outRange = outMax - outMin; // Range of the output value

   // Normalize the input value between 0 and 1
  double normalized = (input - inMin) / inRange;

  // Calculate an output value between 0 and 1.
  double mapped = 0;
  double base   = (0.5 - normalized) * 2;
  if (normalized < 0.5) {
    mapped = -pow(base, power);
  }
  else if (normalized > 0.5) {
    mapped = pow(-base, power);
  }
  
  mapped = mapped * 0.5 + 0.5;
  
  // Remap the output value to the output range requested
  return mapped * outRange + outMin;
}

int lineMissingTime = 0;

//funtion to drive the robot, takes a speed value between 0 and 255 (0% and 100%) as the top speed
//both motors run at MotorSpeed until it sees a corner, then one wheel is ramped down in speed based on how sharp the corner is. this can be edited to make one wheel slow and the other speed up if need be
//motorLowestSpeed set the lowest speed the motors are allowed to go
void drive(int MotorSpeed, int motorLowestSpeed) {
  // the upper and lower bound values determines what the robot sees as a straight line, these basically adjust the sensitivity of the robot
  double upperBound = 2.75;
  double lowerBound = 4.75;

  //speed of the left and right motor ;P
  int motorSpeed_L = 0;
  int motorSpeed_R = 0;

  //read the line sensors and update the sensorArray value
  //this function can propably be put into the drive function
  double linePos = sensorArray.getLinePos();
  double turnAmount = 0;

  ++lineMissingTime;
  if (lineMissingTime < 175 || sensorArray.lineDetected() || sensorArray.horizontalLineDetected()) {
    //if the sensorArray is between the bounds the robot should go straight.
    //
    //The bounds are controlled by 'sensorPower'. Increasing sensorPower increases the size of the 'deadzone' in the
    //middle of the sensor array.
    double turnAmount = mapNonLinear(linePos, 0.5, 6.5, -1, 1, sensorPower);
    turnAmount        = min(1, max(-1, turnAmount)); // Clamp between -1 and 1

    // If turnAmount is < 0, we want to turn right. Decrease the right motor speed and keep the left motor unchanged.
    // If turnAmount is > 0, we want to turn left. Decrease the left motor speed, and keep the right motor unchanged.
    int speedRange = MotorSpeed - motorLowestSpeed;
    motorSpeed_R = (1 - abs(turnAmount) / 2) * (MotorSpeed + min(turnAmount, 0) * speedRange);
    motorSpeed_L = (1 - abs(turnAmount) / 2) * (MotorSpeed - max(turnAmount, 0) * speedRange);

    if (sensorArray.lineDetected()) {
      lineMissingTime = 0;
    }
    else {
      motorSpeed_R /= 2;
      motorSpeed_L /= 2;
    }
  }
  else {
    motorSpeed_R = 0;
    motorSpeed_L = 0;
  }
  //updating the motors with their new speeds
  analogWrite(motors_R, motorSpeed_R);
  analogWrite(motors_L, motorSpeed_L);
  
  Serial.print(" ");
  Serial.print(lineMissingTime);
  Serial.print(" ");
    
  #ifdef PRINT_DEBUG_INFO
  // more debugging serial prints
  Serial.print("     Right motor speed: ");
  Serial.print(motorSpeed_R);
  Serial.print("     Left motor speed: ");
  Serial.print(motorSpeed_L);
  
  Serial.print("     Turn amount: ");
  Serial.print(turnAmount);
  
  #endif
}



void setup() {
  //enabling serial
  Serial.begin(9600);
  Serial.print("Ready...");

  
  SensorConfig sensorConf =
  {
    14,
    { 15, 2, 1, 0, 8, 7, 29, 6 },
    150
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

void loop() {
  //am havign a weird issue where the left motor direction reverts to backwards even though I've set it to forwards
  //setting to forwards every loop seems to fix it for now though
  pinMode(17, OUTPUT);
  digitalWrite(17, LOW);

  //drive the robot
  drive(motorSpeed, 2);
  
  //print a new line on serial so it looks nice
  #ifdef PRINT_DEBUG_INFO
  Serial.println();
  #endif
}
