#include "Commands.h"
#include <string.h>

Commands::CmdDef::CmdDef(const char *_name, CommandFunc _func)
  : name(_name)
  , func(_func)
{}

Commands::Commands(CmdDef * pCommands, uint32_t numCommands, VarDef * pVariables, uint32_t numVariables)
  : m_pCommands(pCommands)
  , m_pVars(pVariables)
  , m_numCommands(numCommands)
  , m_numVars(numVariables)
{}

bool Commands::call(char const * name) const {
  // Get the command definition
  CmdDef *pCmd = getCommand(name);
  if (!pCmd)
    return false; // Command wasn't found

  // Call the function
  pCmd->func();
  return true;
}

bool Commands::hasCommand(char const * name) const {
  return getCommand(name) != 0;
}

bool Commands::hasVariable(char const * name) const {
  return getVariable(name) != 0;
}

int Commands::getVariableType(char const * name) const {
  VarDef *pDef = getVariable(name);
  return pDef ? pDef->typeID : -1;
}

Commands::VarDef* Commands::getVariable(char const * name) const {
  for (uint32_t i = 0; i < m_numVars; ++i)
    if (strcmp(m_pVars[i].name, name) == 0)
      return m_pVars + i;
  return nullptr;
}

Commands::CmdDef* Commands::getCommand(char const * name) const {
  for (uint32_t i = 0; i < m_numCommands; ++i)
    if (strcmp(m_pCommands[i].name, name) == 0)
      return m_pCommands + i;
  return nullptr;
}

int Commands::getVariableCount() const
{
  return m_numVars;
}

int Commands::getCommandCount() const
{
  return m_numCommands;
}
  
char const * Commands::getVariableName(int index) const
{
  return m_pVars[index].name;
}

char const * Commands::getVariableTypeName(int index) const
{
  return m_pVars[index].typeName;    
}

char const * Commands::getVariableTypeName(char const * name) const
{
  VarDef *pDef = getVariable(name);
  return pDef ? pDef->typeName : "none";    
}

char const * Commands::getCommandName(int index) const
{
  return m_pCommands[index].name;  
}
