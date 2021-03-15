#include "Framework.h"
#include "Commands.h"

Test(Commands_Variable)
{
  int thing = 10;

  Commands::VarDef varList[] = {
    { "thing", thing }
  };

  Commands cmds(nullptr, 0, varList, ArraySize(varList));

  Test_Assert(cmds.hasVariable("thing"));
  Test_Assert(cmds.set<int>("thing", 20));

  int var = 0;
  Test_Assert(cmds.get<int>("thing", &var));
  Test_Assert(var == 20);
}

Test(Commands_Command)
{
  static uint32_t test = 0;

  Commands::CmdDef cmdList[] = {
    { "inc-test", []() { test++; } },
    { "dec-test", []() { test--; } }
  };

  Commands cmds(cmdList, ArraySize(cmdList), nullptr, 0);

  Test_Assert(cmds.hasCommand("inc-test"));
  Test_Assert(cmds.hasCommand("dec-test"));

  Test_Assert(test == 0);
  cmds.call("inc-test");
  Test_Assert(test == 1);
  cmds.call("dec-test");
  Test_Assert(test == 0);
}
