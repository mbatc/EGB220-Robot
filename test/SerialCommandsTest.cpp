#include "Framework.h"
#include "SerialCommands.h"
#include "Commands.h"

bool  testFlag  = false;
int   testInt   = 10;
float testFloat = 2.0f;

void setTestFlag() {
  testFlag = true;
}

void clearTestFlag() {
  testFlag = false;
}

Commands::VarDef varDefs[] = {
  { "testFlag",  testFlag  },
  { "testInt",   testInt   },
  { "testFloat", testFloat }
};

Commands::CmdDef cmdDefs[] = {
  { "setFlag", setTestFlag },
  { "clearFlag", clearTestFlag },
};

Commands cmdList(cmdDefs, ArraySize(cmdDefs), varDefs, ArraySize(varDefs));

Test(SerialCommands_Set)
{
  SerialCommands serialCmds(&cmdList);

  testFlag = false;
  testInt = 0;
  testFloat = 0;

  Test_Assert(testFlag == false);
  Test_Assert(testInt == 0);
  Test_Assert(testFloat == 0);

  Test_Assert(serialCmds.execute("set testInt 20") == RT_Set);
  Test_Assert(testInt == 20);
  Test_Assert(serialCmds.execute("set testFlag 1") == RT_Set);
  Test_Assert(testFlag == true);
  Test_Assert(serialCmds.execute("set testFloat 1.2") == RT_Set);
  Test_Assert(abs(testFloat - 1.2) < FLT_EPSILON);

  Test_Assert(serialCmds.execute("set doesntExist 10") == RT_Failure);
}

Test(SerialCommands_Get)
{
  SerialCommands serialCmds(&cmdList);

  testFlag = true;
  testInt = 10;
  testFloat = 1.2;

  Test_Assert(serialCmds.execute("get testInt") == RT_Get);
  Test_Assert(serialCmds.returnType() == TypeID<int>());
  Test_Assert(serialCmds.returnBuffer<int>() == 10);

  Test_Assert(serialCmds.execute("get testFlag") == RT_Get);
  Test_Assert(serialCmds.returnType() == TypeID<bool>());
  Test_Assert(serialCmds.returnBuffer<bool>() == true);

  Test_Assert(serialCmds.execute("get testFloat") == RT_Get);
  Test_Assert(serialCmds.returnType() == TypeID<float>());
  Test_Assert(abs(serialCmds.returnBuffer<float>() - 1.2) < FLT_EPSILON);

  Test_Assert(serialCmds.execute("get doesntExist") == RT_Failure);
}

Test(SerialCommands_Call)
{
  SerialCommands serialCmds(&cmdList);

  Test_Assert(serialCmds.execute("call setFlag") == RT_Call);
  Test_Assert(testFlag == true);
  Test_Assert(serialCmds.execute("call clearFlag") == RT_Call);
  Test_Assert(testFlag == false);

  Test_Assert(serialCmds.execute("call doesntExist") == RT_Failure);
}

Test(SerialCommands_Type)
{
  SerialCommands serialCmds(&cmdList);

  Test_Assert(serialCmds.execute("type testInt") == RT_Type);
  Test_Assert(serialCmds.returnType() == TypeID<int>());
  Test_Assert(serialCmds.returnBuffer<int>() == TypeID<int>());

  Test_Assert(serialCmds.execute("type testFlag") == RT_Type);
  Test_Assert(serialCmds.returnType() == TypeID<int>());
  Test_Assert(serialCmds.returnBuffer<int>() == TypeID<bool>());

  Test_Assert(serialCmds.execute("type testFloat") == RT_Type);
  Test_Assert(serialCmds.returnType() == TypeID<int>());
  Test_Assert(serialCmds.returnBuffer<int>() == TypeID<float>());

  Test_Assert(serialCmds.execute("type doesntExist") == RT_Failure);
}

Test(SerialCommands_Write)
{
  SerialCommands serialCmds(&cmdList);

  bool success = false;
  auto addCall = [&]() {
    success = false;
    std::string cmd = "call setFlag";
    for (char c : cmd)
      Test_Assert(serialCmds.write(c) == RT_NotReady);
    success = true;
  };

  addCall();
  if (!success) return;
  Test_Assert(serialCmds.write('\n') == RT_Call);

  addCall();
  if (!success) return;
  Test_Assert(serialCmds.write('\r') == RT_Call);

  addCall();
  if (!success) return;
  Test_Assert(serialCmds.write('\0') == RT_Call);
}
