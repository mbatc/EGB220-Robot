// #include "MotorDriver.h"
#include "avr/io.h"



// MotorDriver leftMotor('b', 0);
// MotorDriver rightMotor('d', 1);

void setup() {
 // leftMotor.Start();
 // rightMotor.Start();
 // leftMotor.SetSpeed(0.5);
 // rightMotor.SetSpeed(0.5);

  //B7 = DDRB7;
  

 
}

void loop() {
  // DDRB  &= 0; 
  // DDRD  &= 0;
  // DDRB  |= (1 << 7); 
  // DDRD  |= (1 << 0);

  int delay1 = 10;
  int delay2 = 2;

  PORTB |= (1 << 7);
  PORTD |= (1 << 0);
  delay(delay2);
  PORTB = 0;
  PORTD = 0;
  delay(delay1 - delay2);
  
  // delay(500);

  // PORTB =0;
  // PORTD =0;

  
  // PORTD |= (1 << 0); 

  // delay(700);
  // PORTB =0;
  // PORTD =0; 
  //delay(500);

  // analogWrite(4,500);
}
