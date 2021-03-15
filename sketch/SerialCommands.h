#ifndef SerialCommands_h__
#define SerialCommands_h__

#include "Seeker.h"
#include "Commands.h"

// class Commands; // Pre-declare the Commands class

/**
 * Serial Commands Spec:
 * 
 * General Syntax - 
 *  action identified [value]
 *
 * To call a command function:
 *   call myFunc
 *
 * To set a variable:
 *   set myVar 1
 *
 * To get a variable:
 *   get myVar
 *
 * To get a variable type:
 *   type myVar
 *
 * Gets will add data to the SerialCommands read buffer.
 * This can be read from using the read() function.
 */

enum ResultType {
  RT_Failure,
  RT_NotReady,
  RT_Call,
  RT_Set,
  RT_Get,
  RT_Type,
  RT_Count,
};

class SerialCommands
{
public:
  static char const * callToken;
  static char const * setToken;
  static char const * getToken;
  static char const * typeToken;

  /**
   * Create a SerialCommands interface using a set of Commands.
   */
  SerialCommands(Commands *pCommands);
  
  ResultType write(uint8_t byte);
  
  /**
   * Parse and execute a command
   */
  ResultType execute(char const * command);

  uint32_t returnType();

  void* returnBuffer();

  template<typename T>
  T returnBuffer(T defaultVal = T{ 0 }) {
    if (returnType() != TypeID<T>())
      return defaultVal;
    return *(T*)returnBuffer();
  }

protected:
  ResultType executeCall(Seeker *pCmd);
  ResultType executeSet(Seeker *pCmd);
  ResultType executeGet(Seeker *pCmd);
  ResultType executeType(Seeker *pCmd);

  template<typename T>
  ResultType setReturnVar(Seeker *pCmd) {
    T val;
    if (m_pCommands->get<T>(pCmd->getToken(), &val))
      return setReturnVar(val);
    return RT_Get;
  }

  template<typename T>
  ResultType setReturnVar(T const & value) {
    *(T*)m_conversionBuffer = value;
    m_convertedType = TypeID<T>();
    return RT_Get;
  }

  Commands *m_pCommands = nullptr;

  List<uint8_t> m_commandBuffer;

  int     m_convertedType = -1;
  uint8_t m_conversionBuffer[4] = { 0 };
};

#endif // SerialCommands_h__
