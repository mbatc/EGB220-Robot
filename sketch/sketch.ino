#include "ExampleModule.h"

MyModule module(2);

void setup() {
  // put your setup code here, to run once:
  Serial.write(module.GetValue());
}

void loop() {
  // put your main code here, to run repeatedly:

}
