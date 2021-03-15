// #include "MotorDriver.h"
#include "SerialCommands.h"
#include "Commands.h"
#include "SoftwareSerial.h"
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

SoftwareSerial bt(2, 3);

void setup() {
  Serial.begin(9600);
  bt.begin(9600);
  
  commands.execute("set someParameter 1");
  cmdSet.set<int>("someParameter", 1);
  cmdSet.call("someCommand");
  cmdSet.set<int>("someParameter", 10);
  cmdSet.call("someCommand");
}

void writeCommand(uint8_t b) {
  int type = -1;
  
  switch (commands.write(b)) {
  case RT_Get:
    bt.print("Value is ");
    type = commands.returnType();
    if (type == TypeID<int>()) {
      bt.print(commands.returnBuffer<int>());
    }
    else if (type == TypeID<float>()) {
      char buffer[48] = {0};
      sprintf(buffer, "%f", commands.returnBuffer<float>());
      bt.print(buffer);
    }
    else if (type == TypeID<bool>()) {
      bt.print(commands.returnBuffer<bool>());
    }
    else {    
      bt.print("Unknown");
    }
    bt.print("\n");
    break;
  case RT_Type:
    bt.print("Type is ");
    bt.print(commands.returnBuffer<int>());
    bt.print("\n");
    break;
  case RT_Call:
    bt.print("Called function\n");
    break;
  case RT_Set:
    bt.print("Set variable\n");
    break;
  case RT_Failure:
    bt.print("Bad command\n");
    break;
  }
}

void loop() {
  if (bt.available()) {
    Serial.write(bt.peek());
    writeCommand(bt.read());
  }

  if (Serial.available()) {
    writeCommand(Serial.read());
  }
}
