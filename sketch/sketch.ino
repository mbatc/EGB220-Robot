const int IR_LEDS = 14; // ir led pin
const int IR_REC[8] = {16, 2, 1, 0, 8, 7, 29, 6}; // ir receiver pins
int irValue[8]; // array to hold received ir value
const int motors_R = 3; // Right motor
const int motors_L = 11; // Left motor
const int motorsDir[2] = {17, 60}; // motor direction pins **** not pin 60**** just havent been bothered to find it yet
double sensorArray; // double to hold the sensor array equation value


// function to determine where the line is
void getLinePos() {
  // read ir values and store them
  for (int j = 0; j < 8; j++) {
    irValue[j] = analogRead(IR_REC[j]);
  }
  
  //weighted sensor value equation
  sensorArray = (10000 * irValue[0] + 20000 * irValue[1] + 30000 * irValue[2] + 40000 * irValue[3] + 50000 * irValue[4] + 60000 * irValue[5] + 70000 * irValue[6] + 80000 * irValue[7]) / (irValue[0] + irValue[1] + irValue[2] + irValue[3] + irValue[4] + irValue[5] + irValue[6] + irValue[7]);

  // de-bugging serial print, can be comented out
  Serial.print("      Sensor array value: ");
  Serial.print(sensorArray);
  Serial.print("     ir receiver values:");

  // more debugging serial prints
  for (int j = 0; j < 8; j++) {
    Serial.print("  ");
    Serial.print(irValue[j]);

  }
}





//funtion to drive the robot, takes a speed value between 0 and 255 (0% and 100%) as the top speed
//both motors run at MotorSpeed until it sees a corner, then one wheel is ramped down in speed based on how sharp the corner is. this can be edited to make one wheel slow and the other speed up if need be
//motorLowestSpeed set the lowest speed the motors are allowed to go
void drive(int MotorSpeed, int motorLowestSpeed) {

  // the upper and lower bound values determines what the robot sees as a straight line, these basically adjust the sensitivity of the robot
  double upperBound = 41500;
  double lowerBound = 39500;

  // the max and min sensorArray values determine how aggresive the rampdown speed of the motors are when it sees a corner. the closer these are to the upper and lower bound, the 'faster' the robot will try to corner.
  double maxSensorArray = 48000;
  double minSensorArray = 33000;

  //speed of the left and right motor ;P
  int motorSpeed_L;
  int motorSpeed_R;

  //if the sensorArray is between the bounds the robot should go straight
  if (upperBound >= sensorArray && sensorArray >= lowerBound) {
    
    //Go straight

    //motors are both set to max speed as the robot is currently on a straight
    motorSpeed_R = MotorSpeed;
    motorSpeed_L = MotorSpeed;
    
    //updating the motors with their new speeds
    analogWrite(motors_R, motorSpeed_R);
    analogWrite(motors_L, motorSpeed_L);
  }
  //if the sensorArray is greater the the upper bound the robot should turn right
  else if (sensorArray > upperBound) {
    
    //Turn right

    //the map function takes a value, in this case its the sensorArray value, and maps it from one range to another range.
    //here its taking its current value from between the upperBound and maxSensorArray and mapping it to the MotorSpeed and motorLowestSpeed
    motorSpeed_R = map(sensorArray, upperBound, maxSensorArray, MotorSpeed, motorLowestSpeed);
    
    //the left motor is set to the top speed
    motorSpeed_L = MotorSpeed;

    //sometimes the map function will return negative numbers if the sensorArray values is outside of the max, just check if the motorSpeed_R is lower than the lower limit and, if it is, set it to the lower limit
    if (motorSpeed_R < motorLowestSpeed){
      motorSpeed_R = motorLowestSpeed;
    }
    //updating the motors with their new speeds
    analogWrite(motors_R, motorSpeed_R);
    analogWrite(motors_L, motorSpeed_L);

    
  }
  else if (sensorArray < lowerBound) {
    
    //Turn left

    //the map function takes a value, in this case its the sensorArray value, and maps it from one range to another range.
    //here its taking its current value from between the lowerBound and minSensorArray and mapping it to the MotorSpeed and motorLowestSpeed
    motorSpeed_L = map(sensorArray, lowerBound, minSensorArray, MotorSpeed, motorLowestSpeed);
    
    //the left motor is set to the top speed
    motorSpeed_R = MotorSpeed;
    
    //sometimes the map function will return negative numbers if the sensorArray values is outside of the max, just check if the motorSpeed_L is lower than the lower limit and, if it is, set it to the lower limit
    if (motorSpeed_L < motorLowestSpeed){
      motorSpeed_L = motorLowestSpeed;
    }
    //updating the motors with their new speeds
    analogWrite(motors_R, motorSpeed_R);
    analogWrite(motors_L, motorSpeed_L);

    
  }
  // more debugging serial prints
    Serial.print("Right motor speed: ");
    Serial.print(motorSpeed_R);
    Serial.print("       Left motor speed: ");
    Serial.print(motorSpeed_L);

}



void setup() {
  //enabling serial
  Serial.begin(9600);

  //setting pins as inputs or outputs depending on what they are
  pinMode(IR_LEDS, OUTPUT);
  digitalWrite(IR_LEDS, HIGH);

  //ir receivers are inputs
  for (int i = 0; i < 8; i++) {
    pinMode(IR_REC[i], INPUT);
  }

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
  drive(150,2);

  //read the line sensors and update the sensorArray value
  //this function can propably be put into the drive function
  getLinePos();
  
  //print a new line on serial so it looks nice
  Serial.println();


}
