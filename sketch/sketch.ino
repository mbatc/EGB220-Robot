// #include "MotorDriver.h"
#include "SerialCommands.h"
#include "Commands.h"

#include "avr/io.h"

int someParameter = 0;

void doSomething()
{
  Serial.print("You call doSomething! btw, someParameter is ");
  Serial.println(someParameter);
}

Commands::VarDef varList[] = {
  { "someParameter", someParameter }
};

Commands::CmdDef cmdList[] = {
  { "someCommand", doSomething },
};

Commands cmdSet(cmdList, ArraySize(cmdList), varList, ArraySize(varList));

SerialCommands commands(&cmdSet);

void setup() {
  Serial.begin(9600);

  commands.execute("set someParameter 1");
  cmdSet.set<int>("someParameter", 1);
  cmdSet.call("someCommand");
  cmdSet.set<int>("someParameter", 10);
  cmdSet.call("someCommand");
}

void loop() {
  if (Serial.available()) {
    int type = -1;
    switch (commands.write(Serial.read())) {
    case RT_Get:
      Serial.print("Value is ");
      type = commands.returnType();
      if (type == TypeID<int>()) {
        Serial.print(commands.returnBuffer<int>());
      }
      else if (type == TypeID<float>()) {
        char buffer[48] = {0};
        sprintf(buffer, "%f", commands.returnBuffer<float>());
        Serial.print(buffer);
      }
      else if (type == TypeID<bool>()) {
        Serial.print(commands.returnBuffer<bool>());
      }
      else {    
        Serial.print("Unknown");
      }
      Serial.print("\n");
      break;
    case RT_Type:
      Serial.print("Type is ");
      Serial.print(commands.returnBuffer<int>());
      Serial.print("\n");
      break;
    case RT_Call:
      Serial.print("Called function\n");
      break;
    case RT_Set:
      Serial.print("Set variable\n");
      break;
    case RT_Failure:
      Serial.print("Bad command\n");
      break;
    }
  }
}
