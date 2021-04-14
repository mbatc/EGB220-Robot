#ifndef Commands_h__
#define Commands_h__

#include <stdint.h>
#include "Util.h"

/**
 * Commands class for calling functions and accessing variables using strings.
 */
class Commands
{
public:
  typedef void(*CommandFunc)(); // Command function type definition

  /**
   * This struct contains details about a registered command.
   */
  struct CmdDef
  {
    CmdDef(char const *_name, CommandFunc _func);

    char const * name = nullptr;
    CommandFunc func;
  };

  /**
   * This struct contains details about a registered variable
   */
  struct VarDef
  {
    template<typename T>
    VarDef(char const *_name, T &var)
      : name(_name)
      , pVar(&var)
      , typeID(TypeID<T>())
      , typeName(TypeName<T>())
    {}

    char const * name = nullptr;
    void const * pVar = nullptr;
    char const * typeName = nullptr;
    int const typeID   = -1;
  };

  Commands(CmdDef *pCommands, uint32_t numCommands, VarDef *pVariables, uint32_t numVariables);
  Commands(Commands && o) = delete;      // No move
  Commands(Commands const & o) = delete; // No copy

  /**
   * Call a registered commands.
   */
  bool call(char const * name) const;

  /**
   * Check if a command is registered.
   */
  bool hasCommand(char const * name) const;

  /**
   * Check if a variable is registered.
   */
  bool hasVariable(char const * name) const;

  /**
   * Get the number of variables in the command set
   */
  int getVariableCount() const;

  /**
   * Get the number of variables in the command set
   */
  int getCommandCount() const;
  
  /**
   * Get the number of variables in the command set
   */
  char const * getVariableName(int index) const;

  /**
   * Get the type of a variable
   */
  char const * getVariableTypeName(int index) const;
  
  /**
   * Get the type of a variable
   */
  char const * getVariableTypeName(char const * name) const;
  
  /**
   * Get the number of variables in the command set
   */
  char const * getCommandName(int index) const;
  
  /**
   * Set a variable. The type must be the same as the registered
   * variable type.
   *
   * Returns true if the internal variable was set.
   * Returns false otherwise.
   */
  template<typename T>
  bool set(char const * name, T const & var) const {
    VarDef *pAccessor = getVariable(name);
    if (!pAccessor || pAccessor->typeID != TypeID<T>())
      return false;
    *(T*)pAccessor->pVar = var;
    return true;
  }

  /**
   * Get a variable. The type must be the same as the registered
   * variable type.
   *
   * Returns true if the variable was copied into pVar.
   * Returns false otherwise.
   */
  template<typename T>
  bool get(char const * name, T * pVar) const {
    VarDef *pAccessor = getVariable(name);
    if (!pAccessor || pAccessor->typeID != TypeID<T>())
      return false;
    *pVar = *(T*)pAccessor->pVar;
    return true;
  }

  /**
   * Get the type of a registered variable.
   *
   * Returns the type of the variable with the name 'name'.
   * Returns -1 if the variable does not exist.
   */
  int getVariableType(char const * name) const;

protected:
  VarDef* getVariable(char const * name) const;
  CmdDef* getCommand(char const * name) const;

  CmdDef  *m_pCommands   = nullptr;
  uint32_t m_numCommands = 0;

  VarDef  *m_pVars   = nullptr;
  uint32_t m_numVars = 0;
};

#endif // Commands_h__
